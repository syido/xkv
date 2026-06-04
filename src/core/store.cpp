#include <cstddef>
#include <expected>
#include <sstream>
#include <string>

#include "store.hpp"
#include <shared/config.hpp>
#include <core/data/hashtable.hpp>
#include <core/data/xstring.hpp>
#include <core/utils/format.hpp>
#include <string_view>

using namespace std;

namespace xkv {

auto store::have_capacity() const -> bool  {
    return config.capacity_max >= table.get_capacity();
}

auto store::set(string_view key, string_view value, ttl_t ttl) -> app_result {
    if (!have_capacity()) {
        return app_result::out_of_memery;
    } else {
        return table.set(key, xstring{value, ttl});
    }
}

auto store::get(string_view key) -> expected<string_view, app_result> {
    auto result = table.get(key);
    if (result.has_value()) {
        return *result.value();
    } else {
        return unexpected{result.error()};
    }
}

auto store::remove(string_view key) -> app_result {
    return table.remove(key);
}

auto store::compare_and_set(string_view key, string_view old_value, string_view new_value, ttl_t ttl)
    -> app_result {
    auto old_res = table.get(key);
    if (!old_res) {
        return old_res.error();
    } else if (string{*old_res.value()} != old_value) {
        return app_result::constraint;
    } else {
        return set(key, std::move(new_value), ttl);
    }
}

static string_view make_tempview(string str) {
    thread_local static string keeper;
    keeper = std::move(str);
    return string_view{keeper};
}

auto store::info() -> string_view {
    auto snapshot = table.get_info();

    stringstream info;
    info << "size      : " << table.get_size() << '\n'
         << "capacity  : " << capacity_format(table.get_capacity()) << '\n';
    
    size_t bucket_size = pow(2, snapshot.table_size_expr);
    info << "bucket    : "
         << (snapshot.rehash_process == table.NOT_REHASHING ? bucket_size : snapshot.rehash_process) << "/"
         << bucket_size << " (hashing/total)";

    return make_tempview(info.str());
}

auto store::reset_debug() -> void {
    auto ptr = &table;
    table = hashtable<xstring>{};
    cout << ptr;
}

} // namespace xkv
