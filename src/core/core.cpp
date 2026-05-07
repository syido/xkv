#include "core.hpp"
#include "core/config.hpp"
#include <core/init.hpp>
#include <core/io.hpp>

#include <iostream>

using namespace std;

namespace xkv {

void core::print_welcome() {
    cout << "hello, xkv!" << '\n';
}

void core::loop() {
    if (static_config::welcome_msg) {
        print_welcome();
    }

    int fd = io::create_listen(config.port);
    auto handler_func = [this](io *io, const connection &conn) -> void {
        auto [code, str] = handler.handle(conn.inbuf);
        // TODO: 暂时打印
        if (code != app_result::ok) {
            cout << "failed: " << static_cast<int>(code) << endl;
        } else {
            cout << (str.empty() ? "ok" : "ok: ") << str << endl;
        }
    };
    xkv::io{fd, handler_func}.loop();
}

} // namespace xkv
