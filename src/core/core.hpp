#include <core/handler.hpp>
#include <core/io.hpp>

namespace xkv {

// 核心循环
class core {

  private:
    handler handler;

  public:
    core(const core &) = delete;
    core() = default;

    // 启动主循环
    void loop();
};

} // namespace xkv
