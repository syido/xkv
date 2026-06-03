#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace xkv {

// IO句柄类型，在posix上表现为文件描述符fd
// TODO: POSIX fd是int且以-1表示错误，后续平台抽象需要避免无符号句柄直接比较-1。
using io_handle = unsigned long long;

struct connection;
// IO连接实例字典类型
using connection_map = std::unordered_map<io_handle, connection>;

// 连接实例
struct connection {

  public:
    bool closed = false;
    io_handle fd = 0;

    std::string inbuf;
    std::string_view outbuf_view;

  private:
    std::string outbuf;

  public:
    // 构造函数
    explicit connection(io_handle fd);

    // 对outbuf赋值
    void set_outbuf(std::string buf);
    // 重置缓存区
    void reset();
};

class io;
// 回调类型
using on_recv_func = std::function<void(io *io, connection &conn)>;   // TODO: 后面需要检测热点

class io {

  private:
    // 端口
    int port = -1;
    // 接收的回调函数
    on_recv_func on_recv;

    // 所有连接
    std::unordered_map<io_handle, connection> connections;

  public:
    static std::runtime_error io_error;

    io(int port, on_recv_func func);
    ~io() = default; // 析构函数贯穿生命周期，故不需要特殊处理

    // 启动循环
    void loop();

    // 检查回复内容
    static void check_response(connection &conn);
};

} // namespace xkv
