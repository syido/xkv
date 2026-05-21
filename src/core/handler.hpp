#include <core/data/hashtable.hpp>
#include <core/result.hpp>
#include <core/store.hpp>
#include <string>

namespace xkv {

class handler {

  private:
    store main_store;

  public:
    struct result {
        app_result code;
        std::string response;
    };

  public:
    // 解析请求并处理
    result handle(const std::string &request);

    // 从响应中返回字符串
    static std::string make_response(const handler::result &result);
};

} // namespace xkv
