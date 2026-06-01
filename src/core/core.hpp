#include <core/handler.hpp>
#include <core/io.hpp>
#include <core/sync/aof.hpp>

#include <optional>

namespace xkv {

// 核心循环
class core {

  private:
    xkv::handler handler;
    std::optional<xkv::aof> aof;

  public:
    core(const core &) = delete;
    explicit core();

    // 启动主循环
    void loop();
};

} // namespace xkv
