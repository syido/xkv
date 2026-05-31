#pragma once

#include <core/data/types.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace xkv {

// 6字节的无符号整数
struct uint48_t {
    std::array<std::byte, 6> data;

    // 从uint_64得到
    explicit uint48_t(uint64_t val);
    explicit operator uint64_t() const;
};

// 应用内的时间类型
using ttl_t = uint48_t;
// 时间的最大值
inline const ttl_t TTL_MAX = uint48_t{0xFFFFFFFFFFFF};

// 其他信息
using extra_t = uint16_t;
// 空信息
inline const extra_t EXTRA_EMPTY = 0;

// 元数据
struct meta {
    extra_t extra = EXTRA_EMPTY; // 附加信息
    ttl_t ttl = TTL_MAX;         // 过期时间

  public:
    explicit meta() = default;
    explicit meta(ttl_t ttl) : ttl{ttl} {}
};

// xdata基类，作为xkv的value结构
class xdata : noncopyable {

  protected:
    meta meta;

  public:
    xdata(xkv::meta meta) : meta{meta} {}
};

// 作为xkv的字符串，对于小于string的字符串进行压缩
struct xstring : xdata {

  private:
    union {
        std::string str;                            // std::string
        std::array<char, sizeof(std::string)> xstr; // 压缩字符串
    };

  public:
    // 从只读字符串中构造
    explicit xstring(std::string_view str, ttl_t ttl = TTL_MAX);
    xstring(xstring &&);
    xstring &operator=(xstring &&);
    // 判断是否需要析构
    ~xstring();
    operator std::string() const;
    operator std::string_view() const;

    // 是否使用压缩字符串
    bool is_xstr() const;
    // 字符串长度
    size_t size() const;

  public:
    // 使用压缩字符串的标记
    inline static extra_t XSTR_FLAG = 0x0100;
    // 压缩字符串最大长度
    inline static size_t XSTR_MAX_LEN = sizeof(std::string) / sizeof(char) - 1;
};

} // namespace xkv
