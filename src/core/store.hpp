#include <shared/result.hpp>
#include <core/data/hashtable.hpp>
#include <core/data/xstring.hpp>

#include <expected>
#include <string_view>

namespace xkv {

// 存储层，目前仅支持<string, string>
class store {

  private:
    hashtable<xstring> table;

  private:
    bool have_capacity() const;

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
    // 获取信息（低效方法，请勿使用在核心业务）
    std::string_view info();
};

} // namespace xkv
