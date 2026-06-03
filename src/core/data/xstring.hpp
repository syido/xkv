#pragma once

#include <core/data/xdata.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace xkv {

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
    // 获取字符串长度
    size_t get_size() const;
    // 字符串容量
    size_t get_capacity() const override;

  public:
    // 使用压缩字符串的标记
    inline static extra_t NOT_XSTR_FLAG = 0x0100;
    // 压缩字符串最大长度
    inline static size_t XSTR_MAX_LEN = sizeof(std::string) / sizeof(char) - 1;
};

} // namespace xkv
