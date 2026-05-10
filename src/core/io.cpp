#include "io.hpp"
#include "core/result.hpp"

#include <string>
#include <string_view>
#include <utility>

using namespace xkv;
using namespace std;

connection::connection(int fd) : fd{fd} {}

void connection::set_outbuf(string buf) {
    outbuf = std::move(buf);
    outbuf_view = string_view{outbuf};
}

void io::check_response(connection &conn) {
    if (conn.outbuf_view.empty()) {
        conn.set_outbuf(string{static_cast<char>(app_result::loss_response) - '0'});
    }
}

// 其它跨平台实现放在platform/下
