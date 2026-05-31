#pragma once

#include <atomic>
#include <exception>
#include <mutex>
#include <string>
#include <vector>

namespace xkvt::benchmark {

// 基准测试操作类型
enum class operation {
    get, // GET命令
    set  // SET命令
};

// 基准测试命令
struct command {
    operation op;      // 操作类型
    int size = 0;      // 单次操作次数
    int min_len = 100; // 最小数据长度
    int max_len = 300; // 最大数据长度
};

// 命令数组
using commands = std::vector<command>;

// 基准测试配置
struct config {
    int threads = 4;                // IO线程数
    int connections = 100;          // 连接数
    std::string host = "localhost"; // 服务器地址
    std::string port = "12463";     // 服务器端口
};

struct shared_state {
    std::atomic_uint task_count = 0; // 任务计数
    std::mutex error_lock;           // 错误锁
    std::exception_ptr error;        // 当前任务遇到的第一个错误
};

// 基准测试
class benchmark {

  private:
    config conf;                   // 基准测试配置
    std::vector<command> commands; // 本次测试命令

    shared_state state; // 共享状态

  public:
    benchmark(config conf, std::vector<command> commands);

    // 开始测试
    void run();
};

} // namespace xkvt::benchmark
