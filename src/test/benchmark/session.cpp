#include <test/benchmark/benchmark.hpp>

#include <array>
#include <exception>
#include <memory>
#include <random>
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
static void make_request(const command &cmd, string &request);

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

        make_request(cmd, request);

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
    static constexpr size_t pool_size = 16 * 1024 * 1024;
    static const string pool = [] {
        mt19937 generator{random_device{}()};
        uniform_int_distribution<int> dist(0, 255);

        string result;
        result.resize(pool_size);
        for (char &ch : result) {
            ch = static_cast<char>(dist(generator));
        }

        return result;
    }();

    static atomic_size_t offset_cursor = 0;
    static atomic_size_t len_cursor = pool_size - 1;

    size_t len_range = static_cast<size_t>(max_len - min_len + 1);
    size_t len_index = len_cursor.fetch_sub(1, memory_order_relaxed) % pool.size();
    size_t len = static_cast<size_t>(min_len) + static_cast<unsigned char>(pool[len_index]) % len_range;

    request.append(to_string(len));

    size_t offset = offset_cursor.fetch_add(100, memory_order_relaxed) % pool.size();
    if (offset + len <= pool.size()) {
        request.append(pool.data() + offset, len);
        return;
    }

    size_t first_part = pool.size() - offset;
    request.append(pool.data() + offset, first_part);
    request.append(pool.data(), len - first_part);
}

static void make_request(const command &cmd, string &request) {
    request.clear();
    request.push_back(cmd.op == operation::get ? 'g' : 's');
    request.push_back('\r');
    append_random_field(request, cmd.min_len, cmd.max_len);
    request.push_back('\r');

    if (cmd.op == operation::set) {
        append_random_field(request, cmd.min_len, cmd.max_len);
        request.push_back('\r');
    }

    request.push_back('\r');
}

} // namespace
