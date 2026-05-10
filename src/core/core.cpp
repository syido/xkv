#include "core.hpp"
#include "core/config.hpp"
#include <core/init.hpp>
#include <core/io.hpp>

#include <cstddef>
#include <string>

using namespace std;

namespace xkv {

void core::loop() {
    int fd = io::create_listen(config.port);
    auto handler_func = [this](io *io, connection &conn) -> void {
        auto res = handler.handle(conn.inbuf);
        auto buf = handler.make_response(res.first, res.second);
        conn.set_outbuf(buf);
    };

    xkv::io{fd, handler_func}.loop();
}

} // namespace xkv
