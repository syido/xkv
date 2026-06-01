#include "xdata.hpp"

#include <bit>
#include <climits>
#include <cstring>
#include <string>
#include <string_view>

using namespace xkv;
using namespace std;

// 增加字节序的断言
static_assert(CHAR_BIT == 8);
static_assert(std::endian::native == std::endian::little);

uint48_t::uint48_t(uint64_t val) {
    std::memcpy(data.data(), &val, data.size());
}

uint48_t::operator uint64_t() const {
    uint64_t val = 0;
    std::memcpy(&val, data.data(), data.size());
    return val;
}

xstring::xstring(string_view str, ttl_t ttl) : xdata{xkv::meta{ttl}} {
    meta.ttl = ttl;
    if (str.size() <= XSTR_MAX_LEN) {
        meta.extra |= static_cast<extra_t>(str.size());
        std::memcpy(xstr.data(), str.data(), str.size());
        xstr[str.size()] = '\0';
    } else {
        meta.extra |= xstring::NOT_XSTR_FLAG;
        new (&this->str) string{str};
    }
}

// 拷贝构造函数，我们默认string没有自引用，因此直接拷贝地址
xstring::xstring(xstring &&rhs) : xdata{xkv::meta{}} {
    std::swap(xstr, rhs.xstr);
    std::swap(meta, rhs.meta);
}

xstring &xkv::xstring::operator=(xstring &&) {
    return *this;
}

xstring::~xstring() {
    if (!is_xstr()) {
        str.~string();
    }
}

xstring::operator std::string_view() const {
    return is_xstr() ? std::string_view{xstr.data(), size()} : std::string_view{str};
}

xstring::operator string() const {
    return is_xstr() ? std::string{xstr.data(), size()} : str;
}

bool xstring::is_xstr() const {
    return !(meta.extra & xstring::NOT_XSTR_FLAG);
}

size_t xstring::size() const {
    return is_xstr() ? meta.extra & 0x00FFU : str.size();
}
