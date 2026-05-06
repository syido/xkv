#ifdef __APPLE__ // macos

#include <array>
#include <cerrno>
#include <iostream>
#include <stdexcept>

#include <fcntl.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../io.hpp"
#include <core/config.hpp>

namespace xkv {

// 设置fd为非阻塞
static void set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error{"设置fd非阻塞失败"};
    }
}

auto io::io_error = std::runtime_error{"在IO时发生了错误"};

io::io(int listen_fd, on_recv_func func) : listen_fd(listen_fd), on_recv(func) {}

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
                read_connect(fd);
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

        connections[client_fd] = connection{
            // 如果新连接成功，则添加到监听列表
            .closed = false,
            .fd = client_fd,
        };

        struct kevent client_event;
        EV_SET(&client_event, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);

        if (kevent(kq, &client_event, 1, nullptr, 0, nullptr) == -1) { // 加入监听列表
            close(client_fd);
            connections.erase(client_fd);
            throw io_error;
        }
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
        }
    }
}
} // namespace xkv

#endif
