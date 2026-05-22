#pragma once

#include <cstddef>
#include <ctime>
#include <string>

namespace xkv {

struct app_config {
    // 启用AOF（日志追加的持久化）
    bool enable_aof = true;

    // 端口
    int port = 12463;
    // IO缓冲对象池大小（Windows的IOCP在用）
    size_t io_pool_size = 64;

    // 触发扩容的因子
    size_t rehash_fractor = 4;
    // 哈希表中桶最大元素数
    size_t bucket_max = 16;

    // AOF文件路径
    std::string aof_file = "./data/log/";
    // AOF缓冲区大小
    size_t aof_buffer_size = 1024;
    // AOF刷入硬盘的时间间隔
    time_t aof_flush_time_sec = 1;
};

struct static_config {
    // kevent数组的的尺寸
    static constexpr int kevent_size = 64;
    // socket缓冲区的尺寸
    static constexpr size_t buffer_size = 4096;
};

// 全局配置对象
extern const app_config config;

}
