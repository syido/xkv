#include <ctime>
#include <string>

namespace xkv {

// 获取当前时间
inline time_t get_time_sec() {
    // TODO: 需要缓存优化
    return time(nullptr);
}

// 获取当前日期
inline std::string get_date_str() {
    time_t now = get_time_sec();
    char buf[64];
    strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", localtime(&now));
    return std::string(buf);
}

}