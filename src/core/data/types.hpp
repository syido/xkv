#pragma once

#include <concepts>
#include <cstddef>

namespace xkv {

struct xstring; // 预定义

// 当前仅支持string
template <typename E>
concept supported_type = std::same_as<E, xstring>;

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