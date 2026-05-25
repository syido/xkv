#include "aof.hpp"
#include <core/utils/time.hpp>
#include <shared/config.hpp>

#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

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
        file.open(config.aof_file + "aof", ios::out | ios::binary | ios::app);
    } catch (exception const &e) {
        cout << "打开AOF文件出错";
        throw e;
    }

    last_flush = get_time_sec();
}

void aof::append(string_view command) {
    file.write(command.data(), command.size());

    // 如果满足条件就主动刷盘
    if (should_flush()) {
        flush();
    }
}

void aof::load(function<void(const string &)> handler) {
    auto path = config.aof_file + "aof";
    if (!filesystem::exists(path)) {
        return;
    }

    ifstream input(path, ios::in | ios::binary);
    if (!input.is_open()) {
        throw std::runtime_error{"打开AOF文件出错"};
        return;
    }

    string command{};
    char ch = '\0';
    char prev = '\0';
    while (input.get(ch)) {
        command.push_back(ch);

        if (prev == '\r' && ch == '\r') {
            handler(command);
            command.clear();
            prev = '\0';
            continue;
        }

        prev = ch;
    }
}
