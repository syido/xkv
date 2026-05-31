#pragma once

#include <ctime>
#include <string>

namespace xkv {

using uint = unsigned int;

// 获取当前进程启动后的时间偏移，单位秒
uint get_time_sec();

// 获取当前日期
inline std::string get_date_str() {
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", localtime(&now));
    return std::string(buf);
}

}
