#include "core.hpp"
#include <core/io.hpp>
#include <shared/config.hpp>
#include <shared/result.hpp>

#include <string>

using namespace std;

namespace xkv {

core::core() {
    if (config.enable_aof) {
        aof.emplace();
    }
}

void core::loop() {
    // 处理请求的回调函数
    auto handler_func = [this](io *io, connection &conn) -> void {
        auto res = handler.handle(conn.inbuf);

        // 如果命令引起数据变化，则将命令写入aof文件
        if (is_updated(res.code) && config.enable_aof) {
            aof.value().append(conn.inbuf);
        }

        auto buf = handler.make_response(res);
        conn.set_outbuf(buf);
    };

    // 如果启用AOF，则加载旧的AOF文件
    if (config.enable_aof) {
        aof::load([this](const std::string &command) {
            handler.handle(command);
        });
    }

    // 开始IO主循环
    xkv::io{config.port, handler_func}.loop();
}

} // namespace xkv
