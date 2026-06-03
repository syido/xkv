#pragma once

#include <core/data/types.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace xkv {

// 6字节的无符号整数
struct uint48_t {
    std::array<std::byte, 6> data;

    // 从uint_64得到
    explicit uint48_t(uint64_t val);
    operator uint64_t() const;
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
    xkv::meta meta;

  public:
    xdata(xkv::meta meta) : meta{meta} {}

    // 获得占用字节数
    virtual size_t get_capacity() const = 0;
    // 获得过期时间
    ttl_t get_ttl() const;
};

// 容器类
class xcontainer {

  protected:
    size_t size = 0;     // 存放的元素数量
    size_t capacity = 0; // 容器的容量（字节）

  public:
    size_t get_size() const {
        return size;
    }

    size_t get_capacity() const {
        return capacity;
    }
};

} // namespace xkv
