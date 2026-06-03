#pragma once

#include <cstddef>
#include <type_traits>

namespace xkv {

struct xdata; // 预定义

// 当前仅支持string
template <typename E>
concept supported_type = std::is_base_of_v<xdata, E>;

struct noncopyable {
  protected:
    noncopyable() = default;
    ~noncopyable() = default;

    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;

    noncopyable(noncopyable &&) noexcept = default;
    noncopyable &operator=(noncopyable &&) noexcept = default;
};

} // namespace xkv
