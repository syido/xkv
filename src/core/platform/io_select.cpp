#ifdef _WIN32

#include "../io.hpp"
#include <core/config.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

#include <winsock2.h>
#include <ws2tcpip.h>

namespace xkv {

static SOCKET to_socket(int fd) {
    return static_cast<SOCKET>(fd);
}

// 设置socket为非阻塞
static void set_non_blocking(SOCKET socket) {
    u_long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) == SOCKET_ERROR) {
        throw std::runtime_error{"设置socket非阻塞失败"};
    }
}

std::runtime_error io::io_error{"在IO时发生了错误"};

io::io(int listen_fd, on_recv_func func) : listen_fd(listen_fd), on_recv(func) {}

int io::create_listen(int port) {
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        throw std::runtime_error{"winsock startup failed 初始化Winsock失败"};
    }

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error{"listening failed 创建监听socket失败"};
    }

    auto close_and_throw = [listen_socket](const char *message) {
        closesocket(listen_socket);
        WSACleanup();
        throw std::runtime_error{message};
    };

    // 允许服务重启后尽快重新绑定同一端口
    BOOL opt = TRUE;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&opt), sizeof(opt)) ==
        SOCKET_ERROR) {
        close_and_throw("setting reuseaddr failed 设置端口复用失败");
    }

    // 绑定到本机所有IPv4地址和指定端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<u_short>(port));

    // 占用端口
    if (::bind(listen_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        close_and_throw("binding failed 绑定监听端口失败");
    }

    // 进入监听状态，开始接收客户端连接
    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        close_and_throw("lintening failed 启动监听失败");
    }

    return static_cast<int>(listen_socket);
}

void io::loop() {
    SOCKET listen_socket = to_socket(listen_fd);
    set_non_blocking(listen_socket);

    while (true) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(listen_socket, &read_set);

        SOCKET max_socket = listen_socket;
        for (const auto &[fd, conn] : connections) {
            SOCKET socket = to_socket(fd);
            FD_SET(socket, &read_set);
            max_socket = std::max(max_socket, socket);
        }

        int res_size = select(static_cast<int>(max_socket + 1), &read_set, nullptr, nullptr, nullptr);
        if (res_size == SOCKET_ERROR) {
            // TODO: 需要引入日志
            std::cout << "select error 错误" << std::endl;
            continue;
        }

        if (FD_ISSET(listen_socket, &read_set)) { // 新连接
            create_connect(-1);
        }

        std::array<int, FD_SETSIZE> readable_fds{};
        int readable_size = 0;
        for (const auto &[fd, conn] : connections) {
            if (FD_ISSET(to_socket(fd), &read_set)) {
                readable_fds[readable_size++] = fd;
            }
        }

        for (int i = 0; i < readable_size; i++) { // 客户端发来数据
            read_connect(readable_fds[i]);
        }
    }
}

void io::close_connect(int event_fd) {
    if (event_fd == listen_fd) {
        throw io_error;
    } else {
        closesocket(to_socket(event_fd));
        connections.erase(event_fd);
    }
}

void io::create_connect(int) {
    while (true) {
        SOCKET client_socket = accept(to_socket(listen_fd), nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                break;
            } else {
                throw io_error;
            }
        }

        set_non_blocking(client_socket); // client也设为非阻塞

        int client_fd = static_cast<int>(client_socket);
        connections[client_fd] = connection{
            // 如果新连接成功，则添加到监听列表
            .closed = false,
            .fd = client_fd,
        };
    }
}

void io::read_connect(int client_fd) {
    auto it = connections.find(client_fd);
    if (it == connections.end()) { // 找不到连接
        return;
    }

    auto &conn = it->second;

    char buffer[static_config::buffer_size];
    bool received = false;

    while (true) {
        int size = recv(to_socket(client_fd), buffer, static_config::buffer_size, 0);

        if (size > 0) { // 可继续读
            conn.inbuf.append(buffer, size);
            received = true;
        } else if (size == 0) { // 连接关闭
            close_connect(client_fd);
            break;
        } else {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) { // 没有数据可读
                break;
            } else { // 发生错误
                // TODO: 引入日志
                close_connect(client_fd);
                return;
            }
        }
    }

    if (received) { // 读取完毕，回调
        on_recv(this, conn);
    }
}

} // namespace xkv

#endif
