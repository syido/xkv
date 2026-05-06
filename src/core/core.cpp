#include "core.hpp"
#include "core/config.hpp"
#include <core/init.hpp>
#include <core/io.hpp>

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

namespace xkv {

int core::create_listen(int port) {
    // 创建 TCP socket，后续会转为监听 fd。
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        throw std::runtime_error{"listening failed 创建监听socket失败"};
    }

    auto close_and_throw = [listen_fd](const char *message) {
        close(listen_fd);
        throw std::runtime_error{message};
    };

    // 允许服务重启后尽快重新绑定同一端口
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close_and_throw("setting reuseaddr failed 设置端口复用失败");
    }

    // 绑定到本机所有IPv4地址和指定端口
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    // 占用端口
    if (::bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
        close_and_throw("binding failed 绑定监听端口失败");
    }

    // 进入监听状态，开始接收客户端连接
    if (listen(listen_fd, SOMAXCONN) == -1) {
        close_and_throw("lintening failed 启动监听失败");
    }

    return listen_fd;
}

void core::print_welcome() {
    cout << "hello, xkv!" << '\n';
}

void core::loop() {
    if (static_config::welcome_msg) {
        print_welcome();
    }

    int fd = create_listen(config.port);
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
