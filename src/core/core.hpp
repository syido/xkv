#include <core/handler.hpp>
#include <core/io.hpp>
#include <core/sync/aof.hpp>

namespace xkv {

// 核心循环
class core {

  private:
    handler handler;
    aof aof;

  public:
    core(const core &) = delete;
    core() = default;

    // 启动主循环
    void loop();
};

} // namespace xkv
