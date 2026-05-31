#include <expected>
#include <string>

#include "store.hpp"
#include <core/data/hashtable.hpp>
#include <core/data/xdata.hpp>
#include <string_view>

using namespace std;

namespace xkv {

auto store::set(string_view key, string_view value, ttl_t ttl) -> app_result {
    return table.set(key, xstring{value, ttl});
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

} // namespace xkv
