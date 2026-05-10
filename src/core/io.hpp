#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace xkv {

// 连接实例
struct connection {

  public:
    bool closed = false;
    int fd = -1;

    std::string inbuf;
    std::string_view outbuf_view;

  private:
    std::string outbuf;

  public:
    // 构造函数
    explicit connection(int fd);

    // 对outbuf赋值
    void set_outbuf(std::string buf);
};

class io;
// 回调类型
using on_recv_func = std::function<void(io *io, connection &conn)>;   // TODO: 后面需要检测热点

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

    // 创建监听socket
    static int create_listen(int port);

  private:
    // 关闭连接的分支
    void close_connect(int event_fd);
    // 创建连接的分支
    void create_connect(int kq);
    // 读取数据的分支
    void read_connect(int kq, int client_fd);
    // 写入数据的分支
    void write_connect(int kq, int client_fd);

    // 将client_fd加回进写入队列
    void write_back(int kq, int client_fd);

    // 检查回复内容
    void check_response(connection &conn);
};

} // namespace xkv
