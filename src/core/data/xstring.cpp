#include "xstring.hpp"
#include "core/data/xdata.hpp"

#include <cstring>
#include <string>
#include <string_view>

using namespace xkv;
using namespace std;

xstring::xstring(string_view str, ttl_t ttl) : xdata{xkv::meta{ttl}} {
    meta.ttl = ttl;
    if (str.size() <= XSTR_MAX_LEN) {
        meta.extra |= static_cast<extra_t>(str.size());
        memcpy(xstr.data(), str.data(), str.size());
        xstr[str.size()] = '\0';
    } else {
        meta.extra |= xstring::NOT_XSTR_FLAG;
        new (&this->str) string{str};
    }
}

// 拷贝构造函数，我们默认string没有自引用，因此直接拷贝地址
// xstring::xstring(xstring &&rhs) : xdata{xkv::meta{}} {
//     std::swap(xstr, rhs.xstr);
//     std::swap(meta, rhs.meta);
// }

xstring::xstring(xstring &&rhs) : xdata{rhs.meta}, xstr{rhs.xstr} {
    rhs.meta.extra = EXTRA_EMPTY;
}

xstring &xkv::xstring::operator=(xstring &&) {
    return *this;
}

xstring::~xstring() {
    if (!is_xstr()) {
        str.~string();
    }
}

xstring::operator string_view() const {
    return is_xstr() ? string_view{xstr.data(), get_size()} : string_view{str};
}

xstring::operator string() const {
    return is_xstr() ? string{xstr.data(), get_size()} : str;
}

bool xstring::is_xstr() const {
    return !(meta.extra & xstring::NOT_XSTR_FLAG);
}

size_t xstring::get_size() const {
    return is_xstr() ? meta.extra & 0x00FFU : str.size();
}

size_t xstring::get_capacity() const {
    return sizeof(xstring) + (is_xstr() ? 0 : str.capacity());
}
