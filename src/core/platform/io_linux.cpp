#ifdef __linux__ // linux

#include "../io.hpp"
#include <shared/config.hpp>

#include <array>
#include <cerrno>
#include <iostream>
#include <stdexcept>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
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

static void close_connect(connection_map &conns, io_handle listen_fd, io_handle client_fd) {
    if (client_fd == listen_fd) {
        throw io::io_error;
    } else {
        close(client_fd);
        conns.erase(client_fd);
    }
}

static void write_connect(connection_map &conns, int epoll_fd, io_handle listen_fd, io_handle client_fd) {
    auto it = conns.find(client_fd);
    if (it == conns.end()) { // 找不到连接
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
            close_connect(conns, listen_fd, client_fd);
            return;
        }
    }

    if (conn.outbuf_view.empty()) {
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = client_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1) {
            close_connect(conns, listen_fd, client_fd);
        }
    }
}

static void write_back(connection_map &conns, io_handle listen_fd, int client_fd, int epoll_fd) {
    auto it = conns.find(client_fd);
    if (it == conns.end()) {
        return;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1) {
        close_connect(conns, listen_fd, client_fd);
    }
}

static void create_connect(connection_map &conns, io_handle listen_fd, int epoll_fd) {
    while (true) {
        io_handle client_fd = accept(listen_fd, nullptr, nullptr);
        if (client_fd == -1) {
            if (errno == EWOULDBLOCK) {
                break;
            } else {
                throw io::io_error;
            }
        }

        set_non_blocking(client_fd);                     // client也设为非阻塞
        conns.emplace(client_fd, connection{client_fd}); // 如果新连接成功，则添加到监听列表

        struct epoll_event client_event;
        client_event.events = EPOLLIN;
        client_event.data.fd = client_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) { // 加入监听列表
            close(client_fd);
            conns.erase(client_fd);
            throw io::io_error;
        }
    }
}

static void read_connect(connection_map &conns, io_handle listen_fd, io_handle client_fd, int epoll_fd,
                         auto &on_recv, io *io) {
    auto it = conns.find(client_fd);
    if (it == conns.end()) { // 找不到连接
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
            close_connect(conns, listen_fd, client_fd);
            break;
        } else if (errno == EWOULDBLOCK) { // 没有数据可读
            break;
        } else { // 发生错误
            // TODO: 引入日志
            close_connect(conns, listen_fd, client_fd);
            return;
        }

        if (received) { // 读取完毕，回调
            on_recv(io, conn);
            io::check_response(conn);
            write_back(conns, listen_fd, client_fd, epoll_fd);
        }
    }
}

static io_handle create_listen(int port) {
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
    auto listen_fd = create_listen(port);

    int epoll_fd = epoll_create1(0); // 创建epoll句柄
    if (epoll_fd == -1) {
        throw io_error;
    }

    set_non_blocking(listen_fd);

    struct epoll_event event;
    event.events = EPOLLIN; // 填充可读事件
    event.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) { // 并监听
        throw io_error;
    }

    std::array<struct epoll_event, static_config::kevent_size> events;
    while (true) {
        int res_size = epoll_wait(epoll_fd, events.data(), events.size(), -1); // 阻塞等待
        if (res_size == -1) {
            // TODO: 需要引入日志
            std::cout << "epoll error 错误" << std::endl;
        }

        for (int i = 0; i < res_size; i++) { // 遍历所有事件
            auto &event = events[i];
            int fd = event.data.fd;

            if (event.events & EPOLLERR) { // 错误
                throw io_error;
            } else if (fd == listen_fd) { // 新连接
                create_connect(connections, listen_fd, epoll_fd);
            } else if (event.events & EPOLLIN) { // 客户端发来数据
                read_connect(connections, listen_fd, fd, epoll_fd, on_recv, this);
            } else if (event.events & EPOLLOUT) { // 客户端可写
                write_connect(connections, epoll_fd, listen_fd, fd);
            } else {
                // TODO: 未完成
                std::cout << "uncomplete yield 未完成" << std::endl;
            }

            if (event.events & EPOLLHUP) { // 连接关闭
                // TODO: read_connect内部也可能关闭连接，这里后续需要避免重复close。
                close_connect(connections, listen_fd, fd);
            }
        }
    }
}

} // namespace xkv

#endif
