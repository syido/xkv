#ifdef __APPLE__ // macos

#include "../io.hpp"
#include <core/config.hpp>

#include <array>
#include <cerrno>
#include <iostream>
#include <stdexcept>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

namespace xkv {

// 设置fd为非阻塞
static void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error{"设置fd非阻塞失败"};
    }
}

std::runtime_error io::io_error{"在IO时发生了错误"};

io::io(int listen_fd, on_recv_func func) : listen_fd(listen_fd), on_recv(func) {}

int io::create_listen(int port) {
    // 创建 TCP socket，后续会转为监听 fd。
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        throw std::runtime_error{"listening failed 创建监听socket失败"};
    }

    auto close_and_throw = [listen_fd](const char *message) {
        close(listen_fd);
        throw std::runtime_error{message};
    };

    // 允许服务重启后尽快重新绑定同一端口
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close_and_throw("setting reuseaddr failed 设置端口复用失败");
    }

    // 绑定到本机所有IPv4地址和指定端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    // 占用端口
    if (::bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        close_and_throw("binding failed 绑定监听端口失败");
    }

    // 进入监听状态，开始接收客户端连接
    if (listen(listen_fd, SOMAXCONN) == -1) {
        close_and_throw("lintening failed 启动监听失败");
    }

    return listen_fd;
}

void io::loop() {
    int kq = kqueue(); // 创建kqueue句柄
    if (kq == -1) {
        throw io_error;
    }

    set_non_blocking(listen_fd);

    struct kevent event;
    EV_SET(&event, listen_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr); // 填充可读事件
    if (kevent(kq, &event, 1, nullptr, 0, nullptr) == -1) {                    // 并监听
        throw io_error;
    }

    std::array<struct kevent, static_config::kevent_size> events;
    while (true) {
        int res_size = kevent(kq, nullptr, 0, events.data(), events.size(), nullptr); // 阻塞等待
        if (res_size == -1) {
            // TODO: 需要引入日志
            std::cout << "kevent error 错误" << std::endl;
        }

        for (int i = 0; i < res_size; i++) { // 遍历所有事件
            auto &event = events[i];
            int fd = event.ident;

            if (event.flags & EV_ERROR) { // 错误
                throw io_error;
            } else if (fd == listen_fd) { // 新连接
                create_connect(kq);
            } else if (event.filter == EVFILT_READ) { // 客户端发来数据
                read_connect(kq, fd);
            } else if (event.filter == EVFILT_WRITE) { // 客户端可写
                write_connect(kq, fd);
            } else {
                // TODO: 未完成
                std::cout << "uncomplete yield 未完成" << std::endl;
            }

            if (event.flags & EV_EOF) { // 连接关闭
                close_connect(fd);
            }
        }
    }
}

void io::close_connect(int event_fd) {
    if (event_fd == listen_fd) {
        throw io_error;
    } else {
        close(event_fd);
        connections.erase(event_fd);
    }
}

void io::create_connect(int kq) {
    while (true) {
        int client_fd = accept(listen_fd, nullptr, nullptr);
        if (client_fd == -1) {
            if (errno == EWOULDBLOCK) {
                break;
            } else {
                throw io_error;
            }
        }

        set_non_blocking(client_fd); // client也设为非阻塞
        connections.emplace(client_fd, connection{client_fd}); // 如果新连接成功，则添加到监听列表

        struct kevent client_event;
        EV_SET(&client_event, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);

        if (kevent(kq, &client_event, 1, nullptr, 0, nullptr) == -1) { // 加入监听列表
            close(client_fd);
            connections.erase(client_fd);
            throw io_error;
        }
    }
}

void io::read_connect(int kq, int client_fd) {
    auto it = connections.find(client_fd);
    if (it == connections.end()) { // 找不到连接
        return;
    }

    auto &conn = it->second;

    char buffer[static_config::buffer_size];
    bool received = false;

    while (true) {
        ssize_t size = read(client_fd, buffer, static_config::buffer_size);

        if (size > 0) { // 可继续读
            conn.inbuf.append(buffer, size);
            received = true;
        } else if (size == 0) { // 连接关闭
            close_connect(client_fd);
            break;
        } else if (errno == EWOULDBLOCK) { // 没有数据可读
            break;
        } else { // 发生错误
            // TODO: 引入日志
            close_connect(client_fd);
            return;
        }

        if (received) { // 读取完毕，回调
            on_recv(this, conn);
            check_response(conn);
            write_back(kq, client_fd);
        }
    }
}

void io::write_connect(int kq, int client_fd) {
    auto it = connections.find(client_fd);
    if (it == connections.end()) { // 找不到连接
        return;
    }

    auto &conn = it->second;

    while (!conn.outbuf_view.empty()) {
        ssize_t size = write(client_fd, conn.outbuf_view.data(), conn.outbuf_view.size());

        if (size > 0) { // 写入成功，移动待写视图
            conn.outbuf_view.remove_prefix(static_cast<size_t>(size));
        } else if (errno == EWOULDBLOCK) { // 暂时不可写，等待下一次写事件
            break;
        } else { // 发生错误
            // TODO: 引入日志
            close_connect(client_fd);
            return;
        }
    }

    if (conn.outbuf_view.empty()) {
        struct kevent event;
        EV_SET(&event, client_fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);

        if (kevent(kq, &event, 1, nullptr, 0, nullptr) == -1) {
            close_connect(client_fd);
        }
    }
}

void io::write_back(int kq, int client_fd) {
    auto it = connections.find(client_fd);
    if (it == connections.end()) {
        return;
    }

    struct kevent event;
    EV_SET(&event, client_fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);

    if (kevent(kq, &event, 1, nullptr, 0, nullptr) == -1) {
        close_connect(client_fd);
    }
}

} // namespace xkv

#endif
