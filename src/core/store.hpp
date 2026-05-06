#include <core/data/hashtable.hpp>
#include <string>

namespace xkv {

// 存储层，目前仅支持<string, string>
class store {

  private:
    hashtable<std::string> table;

  public:
    // 赋值
    app_result set(const std::string& key, std::string value);
    // 获取
    std::expected<std::string, app_result> get(const std::string& key);
    // 删除
    app_result remove(const std::string& key);
    // CAS，需要满足旧值存在且相等才设置
    app_result compare_and_set(const std::string& key, const std::string& old_value, std::string new_value);
};

} // namespace xkv