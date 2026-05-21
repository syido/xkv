#include "aof.hpp"
#include <core/config.hpp>
#include <core/utils/time.hpp>

#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>

using namespace xkv;
using namespace std;

bool aof::should_flush() {
    return get_time_sec() - last_flush >= config.aof_flush_time_sec;
}

void aof::flush() {
    file.flush();
    last_flush = get_time_sec();
}

aof::aof() {
    try {
        filesystem::create_directories(config.aof_file);
        file.open(config.aof_file + "aof", ios::out);
    } catch (exception const &e) {
        cout << "打开AOF文件出错";
        throw e;
    }

    last_flush = get_time_sec();
}

void aof::append(std::string_view command) {
    file.write(command.data(), command.size());

    // 如果满足条件就主动刷盘
    if (should_flush()) {
        flush();
    }
}
