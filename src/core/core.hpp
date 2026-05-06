#include <core/handler.hpp>
#include <core/io.hpp>

namespace xkv {

// 核心循环
class core {

  private:
    handler handler;

  private:
    static int create_listen(int port);

  public:
    core(const core &) = delete;
    core() = default;

    // 打印欢迎信息
    void print_welcome();
    // 启动主循环
    void loop();
};

} // namespace xkv