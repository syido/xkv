#include "xdata.hpp"

#include <bit>
#include <climits>
#include <cstring>

using namespace xkv;

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

ttl_t xdata::get_ttl() const {
    return meta.ttl;
}
