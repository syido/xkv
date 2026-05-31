#include <core/data/hashtable.hpp>
#include <core/data/xdata.hpp>

#include <string_view>

namespace xkv {

// 存储层，目前仅支持<string, string>
class store {

  private:
    hashtable<xstring> table;

  public:
    // 赋值
    app_result set(std::string_view key, std::string_view value, ttl_t ttl = TTL_MAX);
    // 获取
    std::expected<std::string_view, app_result> get(std::string_view key);
    // 删除
    app_result remove(std::string_view key);
    // CAS，需要满足旧值存在且相等才设置
    app_result compare_and_set(std::string_view key, std::string_view old_value, std::string_view new_value,
                               ttl_t ttl = TTL_MAX);
};

} // namespace xkv
