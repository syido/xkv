#pragma once

#include <atomic>
#include <string>
#include <sys/types.h>
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

// 基准测试
class benchmark {

  private:
    std::atomic_uint count = 0; // 任务计数

    config conf;                   // 基准测试配置
    std::vector<command> commands; // 本次测试命令

  public:
    benchmark(config conf, std::vector<command> commands);

    // 开始测试
    void run();
};

} // namespace xkvt::benchmark
