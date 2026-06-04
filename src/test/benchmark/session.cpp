#include <test/benchmark/benchmark.hpp>
#include <test/shared/utils.hpp>

#include <array>
#include <exception>
#include <memory>
#include <string>
#include <system_error>

#include <asio.hpp>

namespace {

using namespace std;
using namespace xkvt::benchmark;

using ushort = unsigned short;
using uint = unsigned int;

// 向请求中追加一个随机二进制字段
static void append_random_field(string &request, ushort min_len, ushort max_len);

// 根据命令生成一条协议请求
static void make_request(char op, const command &cmd, string &request);

// 一个session代表一个异步TCP连接
class session : public enable_shared_from_this<session> {

  private:
    // 当前连接的socket
    asio::ip::tcp::socket socket;
    // resolver解析出的第一个服务端地址
    asio::ip::tcp::endpoint endpoint;
    // 当前任务使用的命令配置
    const command &cmd;
    // benchmark共享状态
    shared_state &state;
    // 响应读取缓冲区
    array<char, 4096> buffer{};
    // 当前连接复用的请求缓冲区
    string request;
    // 当前连接在ops序列中的位置
    uint op_index = 0;

  public:
    // 创建一个异步连接session
    session(asio::io_context &context, asio::ip::tcp::endpoint endpoint, const command &cmd, shared_state &state)
        : socket(context), endpoint(endpoint), cmd(cmd), state(state) {
        request.reserve(static_cast<size_t>(cmd.max_len) * 2 + 64);
    }

    // 启动连接
    void start() {
        auto self = shared_from_this();
        auto on_connect = [self](const asio::error_code &error) {
            if (!error) {
                self->send_next();
            } else if (error != asio::error::operation_aborted) {
                self->fail(make_exception_ptr(system_error(error, "connect failed")));
            }
        };

        socket.async_connect(endpoint, on_connect);
    }

  private:
    // 记录错误，并让其他连接自然退出
    void fail(exception_ptr error) {
        {
            lock_guard lock{state.error_lock};
            if (!state.error) {
                state.error = error;
                state.task_count.store(0, memory_order_relaxed);
            }
        }

        asio::error_code ignored;
        socket.close(ignored);
    }

    // 抢占一个请求额度并异步发送
    void send_next() {
        uint current = state.task_count.load(memory_order_relaxed);
        while (current != 0 &&
               !state.task_count.compare_exchange_weak(current, current - 1, memory_order_relaxed)) {}

        if (current == 0) {
            socket.close();
            return;
        }

        uint set_count = op::set_count(cmd.ops);
        uint total_count = op::total_count(cmd.ops);
        char current_op = op_index < set_count ? 's' : 'g';
        ++op_index;
        if (op_index == total_count) {
            op_index = 0;
        }

        make_request(current_op, cmd, request);

        auto self = shared_from_this();
        auto on_write = [self](const asio::error_code &error, size_t) {
            if (!error) {
                self->read_response();
            } else if (error != asio::error::operation_aborted) {
                self->fail(make_exception_ptr(system_error(error, "write failed")));
            }
        };

        asio::async_write(socket, asio::buffer(request), on_write);
    }

    // 异步读取服务端响应
    void read_response() {
        auto self = shared_from_this();
        auto on_read = [self](const asio::error_code &error, size_t) {
            if (!error) {
                self->send_next();
            } else if (error != asio::error::operation_aborted) {
                self->fail(make_exception_ptr(system_error(error, "read failed")));
            }
        };

        socket.async_read_some(asio::buffer(buffer), on_read);
    }
};

static void append_random_field(string &request, ushort min_len, ushort max_len) {
    string field = xkvt::get_random_str(min_len, max_len);
    request.append(to_string(field.size()));
    request.append(field);
}

static void make_request(char op, const command &cmd, string &request) {
    request.clear();
    request.push_back(op);
    request.push_back('\r');
    append_random_field(request, cmd.min_len, cmd.max_len);
    request.push_back('\r');

    if (op == 's') {
        append_random_field(request, cmd.min_len, cmd.max_len);
        request.push_back('\r');
    }

    request.push_back('\r');
}

} // namespace
