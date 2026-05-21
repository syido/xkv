#include "core.hpp"
#include "core/config.hpp"
#include "core/result.hpp"
#include <core/io.hpp>

#include <string>

using namespace std;

namespace xkv {

void core::loop() {
    auto handler_func = [this](io *io, connection &conn) -> void {
        auto res = handler.handle(conn.inbuf);

        // 如果命令引起数据变化，则将命令写入aof文件
        if (is_updated(res.code)) {
            aof.append(conn.inbuf);
        }

        auto buf = handler.make_response(res);
        conn.set_outbuf(buf);
    };

    xkv::io{config.port, handler_func}.loop();
}

} // namespace xkv
