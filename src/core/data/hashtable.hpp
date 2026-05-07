#pragma once

#include <core/data/types.hpp>
#include <core/init.hpp>
#include <core/result.hpp>

#include <expected>
#include <functional>
#include <string>
#include <sys/types.h>
#include <tuple>
#include <utility>
#include <vector>

namespace xkv {

// 哈希表类（支持string为key，string或int为value）
template <supported_type E>
class hashtable {

  private:
    using entry_type = std::pair<std::string, E>;           // 键值对类型
    using bucket_type = std::vector<entry_type>;            // 桶类型
    using bucket_iterator = typename bucket_type::iterator; // 桶迭代器

    int rehash_process = NOT_REHASHING;           // rehash已完成的进度，-2表示不在rehash，-1表示即将向扩容
    unsigned int old_size_expr = 0;               // 若在rehash时，旧桶数的幂
    unsigned int table_size_expr = old_size_expr; // 桶数的幂
    size_t size = 0;                              // 表元素的数量
    size_t bytes = 0;                             // TODO: 表占用的字节数（未完成）
    std::vector<bucket_type *> table;             // 桶表

    // 获取元素所在桶、迭代器与用之模的幂（提供给get和remove），并决定找不到是创建还是返回错误（提供给set）
    std::expected<std::tuple<bucket_type &, bucket_iterator>, app_result>
    find(const std::string &key, bool throw_not_found = true);
    // 判断桶是否超载
    bool is_big_to_rehash(const bucket_type &bucket) const;
    // 检查或渐进式重构哈希表，参数代表关联操作是否有对哈希表改动
    app_result check_rehash(bool changed = true, bool too_big = false);

    // 获取字符串的哈希值
    size_t get_hash(const std::string &key) const;
    // 获取哈希值模数
    size_t get_expr_mod(size_t key, size_t expr) const;

  public:
    // 哈希表构造函数
    hashtable();
    // 哈希表析构函数
    ~hashtable();

    // 查询元素
    std::expected<E, app_result> get(const std::string &key);
    // 插入元素
    app_result set(const std::string &key, E value);
    // 删除元素
    app_result remove(const std::string &key);

    // 不在rehash的过程中的常量
    static const int NOT_REHASHING = -2;
};

template <supported_type E>
auto hashtable<E>::find(const std::string &key, bool create_if_not_found)
    -> std::expected<std::tuple<bucket_type &, bucket_iterator>, app_result> {

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

    if (table[mod] != nullptr) {
        auto &vec = *table[mod];
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (it->first == key) {
                return std::tuple<bucket_type &, bucket_iterator>{vec, it};
            }
        }
    } else if (create_if_not_found) {
        table[mod] = new bucket_type;
    } else {
        return std::unexpected(app_result::not_found);
    }

    if (create_if_not_found) {
        // 没有对应桶和没有元素都返回end()
        return std::tuple<bucket_type &, bucket_iterator>{*table[mod], table[mod]->end()};
    } else {
        return std::unexpected(app_result::not_found);
    }
}

template <supported_type E>
auto hashtable<E>::is_big_to_rehash(const bucket_type &bucket) const -> bool {
    return bucket.size() > config.bucket_max;
}

template <supported_type E>
auto hashtable<E>::get_hash(const std::string &key) const -> size_t {
    return std::hash<std::string>{}(key);
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
    for (bucket_type *&vec : table) {
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
                size_t new_mod = get_expr_mod(get_hash(it->first), table_size_expr);
                if (new_mod != old_mod) { // 新模不同于旧模，增到新桶中，从旧桶移出
                    if (table[new_mod] == nullptr) {
                        table[new_mod] = new bucket_type;
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

    return app_result::ok;
}

template <supported_type E>
auto hashtable<E>::get(const std::string &key) -> std::expected<E, app_result> {
    auto const &res = find(key, false);
    if (!res.has_value()) {
        return std::unexpected(res.error());
    } else {
        return std::get<1>(res.value())->second;
    }
}

// TODO: 没写扩容错误
template <supported_type E>
auto hashtable<E>::set(const std::string &key, E value) -> app_result {
    // 注：我们决定新插入数据不总是插入新桶，而是按照rehash进度决定插入哪个桶

    auto res = find(key, true);
    if (!res.has_value()) {
        return res.error();
    }

    auto &[vec, it] = res.value();
    if (it != vec.end()) {             // 找到元素
        it->second = std::move(value); // 替换元素
    } else {
        vec.emplace_back(key, std::move(value));
        ++size;
    }

    return check_rehash(true, is_big_to_rehash(vec));
}

template <supported_type E>
auto hashtable<E>::remove(const std::string &key) -> app_result {
    auto res = find(key, false);
    if (!res.has_value()) {
        return res.error();
    }

    auto &[vec, it] = res.value();
    std::swap(*it, vec.back()); // 将待删除的元素与最后一个元素交换
    vec.pop_back();             // 然后删除最后一个元素
    --size;

    return check_rehash(true);
}

} // namespace xkv
