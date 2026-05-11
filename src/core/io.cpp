#include "io.hpp"
#include "core/result.hpp"

#include <string>
#include <string_view>
#include <utility>

using namespace xkv;
using namespace std;

connection::connection(io_handle fd) : fd{fd} {}

void connection::set_outbuf(string buf) {
    outbuf = std::move(buf);
    outbuf_view = string_view{outbuf};
}

void io::check_response(connection &conn) {
    if (conn.outbuf_view.empty()) {
        conn.set_outbuf(string{static_cast<char>(app_result::loss_response) - '0'});
    }
}

std::runtime_error io::io_error{"在IO时发生了错误"};

io::io(int port, on_recv_func func) : port(port), on_recv(func) {}

// 其它跨平台实现放在platform/下
