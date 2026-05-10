#include <core/data/hashtable.hpp>
#include <core/result.hpp>
#include <core/store.hpp>
#include <utility>
#include <string>

namespace xkv {

class handler {

  private:
    store main_store;

  public:
    // 解析请求并处理
    std::pair<app_result, std::string> handle(const std::string &request);

    // 从响应中返回字符串
    static std::string make_response(app_result result, std::string value);
};

} // namespace xkv
