#include <concepts>
#include <string>

namespace xkv {
// enum data_type {
//     integer,
//     string
// };

// 当前仅支持int和string
template <typename E>
concept supported_type = std::same_as<E, int> || std::same_as<E, std::string>;

}