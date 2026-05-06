#include <string>

#include "store.hpp"
#include <core/data/hashtable.hpp>

using namespace std;

namespace xkv {

auto store::set(const string& key, string value) -> app_result {
    return table.set(key, value);
}

auto store::get(const string &key) -> expected<string, app_result> {
    return table.get(key);
}

auto store::remove(const string &key) -> app_result {
    return table.remove(key);
}

auto store::compare_and_set(const string &key, const string &old_value, string new_value) -> app_result {
    auto old_res = table.get(key);
    if (!old_res) {
        return old_res.error();
    } else if (old_res.value() != old_value) {
        return app_result::constraint;
    } else {
        return table.set(key, new_value);
    }
}

} // namespace xkv