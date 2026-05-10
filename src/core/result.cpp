#include "result.hpp"

using namespace std;

namespace xkv {

string to_string(app_result code) {
    int value = static_cast<int>(code);
    return string{
        static_cast<char>('0' + value / 10),
        static_cast<char>('0' + value % 10),
    };
}

} // namespace xkv
