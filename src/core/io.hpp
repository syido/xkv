#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace xkv {

struct connection {
    bool closed = false;
    int fd = -1;

    std::string inbuf;
    std::string outbuf;
};

class io;
// 回调类型
using on_recv_func = std::function<void(io *io, const connection &conn)>;   // TODO: 后面需要检测热点

class io {

  private:
    // 侦听的fd
    int listen_fd = -1;
    // 接收的回调函数
    on_recv_func on_recv;

    // 所有连接
    std::unordered_map<int, connection> connections;

  public:
    static std::runtime_error io_error;

    io(int listen_fd, on_recv_func func);
    ~io() = default; // 析构函数贯穿生命周期，故不需要特殊处理

    // 启动循环
    void loop();

  private:
    // 关闭连接的分支
    void close_connect(int event_fd);
    // 创建连接的分支
    void create_connect(int kq);
    // 读取数据的分支
    void read_connect(int client_fd);
};

} // namespace xkv
