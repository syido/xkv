#pragma once

#include <cstddef>

namespace xkv {

struct app_config {
    // 端口
    int port = 12463;
    // IO缓冲对象池大小（Windows的IOCP在用）
    size_t io_pool_size = 64;

    // 触发扩容的因子
    size_t rehash_fractor = 4;
    // 哈希表中桶最大元素数
    size_t bucket_max = 16;
};

struct static_config {
    // kevent数组的的尺寸
    static constexpr int kevent_size = 64;
    // socket缓冲区的尺寸
    static constexpr size_t buffer_size = 4096;
};

}
