#include <cstddef>
#include <ctime>
#include <fstream>
#include <string_view>

namespace xkv {

// AOF任务管理类
class aof {

  private:
    std::ofstream file; // 文件实例
    time_t last_flush;  // 最后一次刷盘时间

  private:
    // 检查提交条件
    bool should_flush();
    // 主动刷盘
    void flush();

  public:
    // 构造函数
    aof();

    // 追加命令
    void append(std::string_view command);
};

} // namespace xkv