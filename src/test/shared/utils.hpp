#pragma once

#include <atomic>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <string>

namespace xkvt {

// 生成随机文本
inline std::string get_random_str(size_t minlen, size_t max_len) {
    if (max_len < minlen) {
        throw std::invalid_argument("maxlen不能小于minlen");
    }

    static constexpr size_t pool_size = 16 * 1024 * 1024;
    static const std::string pool = [] {
        std::mt19937 generator{std::random_device{}()};
        std::uniform_int_distribution<int> dist(0, 255);

        std::string result;
        result.resize(pool_size);
        for (char &ch : result) {
            ch = static_cast<char>(dist(generator));
        }

        return result;
    }();

    static std::atomic_size_t offset_cursor = 0;
    static std::atomic_size_t len_cursor = pool_size - 1;

    size_t len_range = max_len - minlen + 1;
    size_t len_index = len_cursor.fetch_sub(1, std::memory_order_relaxed) % pool.size();
    size_t len = minlen + static_cast<unsigned char>(pool[len_index]) % len_range;

    size_t offset = offset_cursor.fetch_add(100, std::memory_order_relaxed) % pool.size();
    if (offset + len <= pool.size()) {
        return std::string{pool.data() + offset, len};
    }

    size_t first_part = pool.size() - offset;

    std::string result;
    result.reserve(len);
    result.append(pool.data() + offset, first_part);
    result.append(pool.data(), len - first_part);
    return result;
}

} // namespace xkvt
