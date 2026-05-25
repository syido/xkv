#include "benchmark.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

#include <asio.hpp>

using namespace xkvt::benchmark;
using namespace std;

#include "session.cpp"

// 返回操作类型的输出名称
static string_view operation_name(operation op);

benchmark::benchmark(config conf, vector<command> commands) : conf(conf), commands(commands) {}

void benchmark::run() {
    for (size_t task_index = 0; task_index < commands.size(); task_index++) {
        const command &cmd = commands[task_index];
        count.store(cmd.size, memory_order_relaxed);

        cout << "task" << task_index + 1 << ": " << operation_name(cmd.op) << ", " << cmd.size << '\n';

        auto start = chrono::steady_clock::now();
        asio::error_code task_error;

        asio::io_context context;
        asio::ip::tcp::resolver resolver{context};
        auto resolved = resolver.resolve(conf.host, conf.port);
        vector<asio::ip::tcp::endpoint> endpoints;
        for (const auto &entry : resolved) {
            endpoints.push_back(entry.endpoint());
        }

        for (int index = 0; index < conf.connections; index++) {
            make_shared<session>(context, endpoints, cmd, count, task_error)->start();
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
        if (task_error) {
            throw runtime_error(task_error.message());
        }

        chrono::duration<double> elapsed = end - start;
        double qps = elapsed.count() == 0 ? 0 : static_cast<double>(cmd.size) / elapsed.count();

        cout << "cost: " << elapsed.count() << "s, QPS: " << qps << '\n';
    }
}

static string_view operation_name(operation op) {
    return op == operation::get ? "get" : "set";
}
