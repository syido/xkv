#include "benchmark.hpp"
#include <test/benchmark/session.cpp>

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <asio.hpp>

using namespace xkvt::benchmark;
using namespace std;

// 输出任务配置
static void print_task(size_t task_index, const command &cmd);
// 试探服务端连接
static void probe_connect(asio::ip::tcp::endpoint endpoint);
// 重置服务端状态
static void send_reset(asio::ip::tcp::endpoint endpoint);

benchmark::benchmark(config conf, vector<command> commands) : conf(conf), commands(commands) {}

void benchmark::run() {
    asio::io_context probe_context;
    asio::ip::tcp::resolver resolver{probe_context};
    auto resolved = resolver.resolve(asio::ip::tcp::v4(), conf.host, conf.port);
    asio::ip::tcp::endpoint endpoint = resolved.begin()->endpoint();
    probe_connect(endpoint);

    for (size_t task_index = 0; task_index < commands.size(); task_index++) {
        const command &cmd = commands[task_index];
        if (op::total_count(cmd.ops) == 0) {
            throw invalid_argument("非法的命令组合");
        }

        if (cmd.reset) {
            send_reset(endpoint);
        }

        state.task_count.store(cmd.size, memory_order_relaxed);
        {
            lock_guard lock{state.error_lock};
            state.error = nullptr;
        }

        print_task(task_index, cmd);

        auto start = chrono::steady_clock::now();

        asio::io_context context;

        for (int index = 0; index < conf.connections; index++) {
            make_shared<session>(context, endpoint, cmd, state)->start();
        }

        vector<thread> threads;
        threads.reserve(conf.threads);

        for (int index = 0; index < conf.threads; index++) {
            threads.emplace_back([&context] {
                context.run();
            });
        }

        for (auto &thread : threads) {
            thread.join();
        }

        auto end = chrono::steady_clock::now();
        {
            lock_guard lock{state.error_lock};
            if (state.error) {
                rethrow_exception(state.error);
            }
        }

        chrono::duration<double> elapsed = end - start;
        double qps = elapsed.count() == 0 ? 0 : static_cast<double>(cmd.size) / elapsed.count();

        cout << "cost: " << elapsed.count() << "s, QPS: " << qps << '\n';
    }
}

static void print_task(size_t task_index, const command &cmd) {
    cout << "task" << task_index + 1 << "> set: " << op::set_count(cmd.ops)
         << ", get: " << op::get_count(cmd.ops) << ", ops: " << cmd.size << '\n';
}

static void probe_connect(asio::ip::tcp::endpoint endpoint) {
    asio::io_context context;
    asio::ip::tcp::socket socket{context};
    asio::steady_timer timer{context};
    asio::error_code connect_error;
    bool connected = false;
    bool timeout = false;

    timer.expires_after(chrono::seconds{5});
    timer.async_wait([&](const asio::error_code &error) {
        if (!error && !connected) {
            timeout = true;

            asio::error_code ignored;
            socket.close(ignored);
        }
    });

    socket.async_connect(endpoint, [&](const asio::error_code &error) {
        connect_error = error;
        connected = !error;

        asio::error_code ignored;
        timer.cancel(ignored);
    });

    context.run();

    if (timeout) {
        throw runtime_error("connect timeout");
    }
    if (connect_error) {
        throw runtime_error(connect_error.message());
    }
}

static void send_reset(asio::ip::tcp::endpoint endpoint) {
    asio::io_context context;
    asio::ip::tcp::socket socket{context};
    socket.connect(endpoint);

    static const string request = "R\r\r";
    asio::write(socket, asio::buffer(request));

    array<char, 64> buffer{};
    size_t n = socket.read_some(asio::buffer(buffer));
    string response{buffer.data(), n};
    if (!response.starts_with("00\r")) {
        throw runtime_error("reset failed: " + response);
    }
}
