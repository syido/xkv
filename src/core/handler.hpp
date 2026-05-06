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
    std::pair<app_result, std::string> handle(const std::string &request);
};

} // namespace xkv
