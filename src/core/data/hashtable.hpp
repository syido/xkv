#pragma once

#include <core/data/types.hpp>
#include <core/data/xdata.hpp>
#include <core/utils/time.hpp>
#include <shared/config.hpp>
#include <shared/result.hpp>

#include <cstddef>
#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <tuple>
#include <utility>
#include <vector>

namespace xkv {

// 哈希表类
template <supported_type E>
class hashtable : public xcontainer {

  private:
    struct entry;                                            // 键值对类型 <std::string, E>
    using bucket = std::vector<entry>;                       // 桶类型
    using bucket_iterator = typename bucket::iterator;       // 桶迭代器
    using entry_pos = std::tuple<bucket &, bucket_iterator>; // 元素位置

    int rehash_process = NOT_REHASHING;           // rehash已完成的进度，-2表示不在rehash，-1表示即将向扩容
    unsigned int old_size_expr = 0;               // 若在rehash时，旧桶数的幂
    unsigned int table_size_expr = old_size_expr; // 桶数的幂
    std::vector<bucket *> table;                  // 桶表

    // 获取元素所在桶、迭代器与用之模的幂（提供给get和remove），并决定找不到是创建还是返回错误（提供给set）
    std::expected<entry_pos, app_result> find(std::string_view key, bool throw_not_found = true);

    // 判断桶是否超载
    bool is_big_to_rehash(const bucket &bucket) const;
    // 检查或渐进式重构哈希表，参数代表关联操作是否有对哈希表改动
    app_result check_rehash(bool changed = true, bool too_big = false);

    // 获取字符串的哈希值
    size_t get_hash(std::string_view key) const;
    // 获取哈希值模数
    size_t get_expr_mod(size_t key, size_t expr) const;

  public:
    // 不在rehash的过程中的常量
    static const int NOT_REHASHING = -2;
    // 状态切片
    struct info;

  public:
    // 哈希表构造函数
    hashtable();
    // 哈希表析构函数
    ~hashtable();

    // 查询元素
    std::expected<const E *, app_result> get(std::string_view key);
    // 插入元素
    app_result set(std::string_view key, E value);
    // 从元素定位中删除元素
    void remove(entry_pos pos);
    // 删除元素
    app_result remove(std::string_view key);
    // 获得哈希表状态快照
    info get_info() const;
};

template <supported_type E>
struct hashtable<E>::entry {
    std::string key;
    E value;

    size_t get_capacity() const {
        return key.capacity() + value.get_capacity();
    }
};

template <supported_type E>
struct hashtable<E>::info {
    int rehash_process;
    unsigned int old_size_expr;
    unsigned int table_size_expr;
    float avg_bucket_size;
};

template <supported_type E>
auto hashtable<E>::find(std::string_view key, bool create_if_not_found)
    -> std::expected<entry_pos, app_result> {

    // 注：我们决定新插入数据不总是插入新桶，而是按照rehash进度决定插入哪个桶
    size_t hash = get_hash(key);
    size_t mod;

    // 如果不在扩容中
    if (rehash_process == NOT_REHASHING) {
        mod = get_expr_mod(hash, table_size_expr);
    } else {
        // 如果在扩容中
        bool is_increase = old_size_expr < table_size_expr;
        // 先用的旧桶求模
        mod = get_expr_mod(hash, old_size_expr);
        if ((is_increase && mod <= static_cast<size_t>(rehash_process)) ||
            (!is_increase && mod >= static_cast<size_t>(rehash_process))) {
            mod = get_expr_mod(hash, table_size_expr);
        }
    }

    // 从桶中遍历得元素
    if (table[mod] != nullptr) {
        auto &vec = *table[mod];
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (std::string_view{it->key} == key) {
                return entry_pos{vec, it};
            }
        }
    } else if (create_if_not_found) {
        table[mod] = new bucket{};
    } else {
        return std::unexpected(app_result::not_found);
    }

    if (create_if_not_found) {
        // 没有对应桶和没有元素都返回end()
        return entry_pos{*table[mod], table[mod]->end()};
    } else {
        return std::unexpected(app_result::not_found);
    }
}

template <supported_type E>
auto hashtable<E>::is_big_to_rehash(const bucket &bucket) const -> bool {
    return bucket.size() > config.bucket_max;
}

template <supported_type E>
auto hashtable<E>::get_hash(std::string_view key) const -> size_t {
    return std::hash<std::string_view>{}(key);
}

template <supported_type E>
auto hashtable<E>::get_expr_mod(size_t key, size_t expr) const -> size_t {
    return key & ((1 << expr) - 1);
}

template <supported_type E>
hashtable<E>::hashtable() : table(1 << old_size_expr) {
    /* 构造 */
}

template <supported_type E>
hashtable<E>::~hashtable() {
    for (bucket *&vec : table) {
        if (vec != nullptr) {
            delete vec;
        }
    }
}

// TODO: 没有处理扩容失败、没有处理缩容
template <supported_type E>
auto hashtable<E>::check_rehash(bool changed, bool too_big) -> app_result {
    if (rehash_process == NOT_REHASHING) { // 仅当不在rehash中时才处理
        if (changed) {
            if (too_big || size > table.size() * config.rehash_fractor) { // 桶太大或超过负载因子则扩容
                ++table_size_expr;
                table.resize(1 << table_size_expr);
                rehash_process = 0;
            }
        }
    }

    if (table_size_expr > old_size_expr) { // 正在扩容中
        size_t old_table_size = 1 << old_size_expr;
        size_t old_mod = static_cast<size_t>(rehash_process);

        if (table[old_mod] != nullptr) {
            auto &old_bucket = *table[old_mod];
            for (auto it = old_bucket.begin(); it != old_bucket.end();) {
                size_t new_mod = get_expr_mod(get_hash(it->key), table_size_expr);
                if (new_mod != old_mod) { // 新模不同于旧模，增到新桶中，从旧桶移出
                    if (table[new_mod] == nullptr) {
                        table[new_mod] = new bucket;
                    }
                    table[new_mod]->emplace_back(std::move(*it));
                    std::swap(*it, old_bucket.back());
                    old_bucket.pop_back();
                } else {
                    ++it;
                }
            }

            if (old_bucket.empty()) {
                delete table[old_mod];
                table[old_mod] = nullptr;
            }
        }

        if (old_mod + 1 >= old_table_size) {
            old_size_expr = table_size_expr;
            rehash_process = NOT_REHASHING;
        } else {
            ++rehash_process;
        }
    }

    return changed ? app_result::updated : app_result::ok;
}

template <supported_type E>
auto hashtable<E>::get(std::string_view key) -> std::expected<const E *, app_result> {
    auto const &res = find(key, false);
    if (!res.has_value()) {
        return std::unexpected{res.error()};
    } else {
        auto &[vec, it] = res.value();
        if (it->value.get_ttl() <= get_time_sec()) {
            remove(res.value());
            return std::unexpected{res.error()};
        } else {
            return &it->value;
        }
    }
}

// TODO: 没写扩容错误
template <supported_type E>
auto hashtable<E>::set(std::string_view key, E value) -> app_result {
    // 注：我们决定新插入数据不总是插入新桶，而是按照rehash进度决定插入哪个桶

    auto res = find(key, true);
    if (!res.has_value()) {
        return res.error();
    }

    auto &[vec, it] = res.value();
    if (it != vec.end()) { // 找到元素
        capacity -= it->get_capacity();
        it->value = std::move(value); // 替换元素
        capacity += it->get_capacity();
    } else {
        vec.emplace_back(std::string{key}, std::move(value));
        capacity += vec.back().get_capacity();
        size += 1;
    }

    return check_rehash(true, is_big_to_rehash(vec));
}

template <supported_type E>
void hashtable<E>::remove(entry_pos pos) {
    auto &[vec, it] = pos;
    std::swap(*it, vec.back()); // 将待删除的元素与最后一个元素交换
    capacity -= vec.back().get_capacity();
    size -= 1;
    vec.pop_back(); // 然后删除最后一个元素
}

template <supported_type E>
auto hashtable<E>::remove(std::string_view key) -> app_result {
    auto res = find(key, false);
    if (!res.has_value()) {
        return res.error();
    }
    remove(res.value());
    return check_rehash(true);
}

template <supported_type E>
auto hashtable<E>::get_info() const -> info {
    return info{
        .rehash_process = rehash_process,
        .old_size_expr = old_size_expr,
        .table_size_expr = table_size_expr,
        .avg_bucket_size = table.size() == 0 ? 0 : (float)size / table.size(),
    };
}

} // namespace xkv
