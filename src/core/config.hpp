#pragma once

#include <cstddef>
#include <unistd.h>

namespace xkv {

struct app_config {
    // 触发扩容的因子
    size_t rehash_fractor = 4;
    // 哈希表中桶最大元素数
    size_t bucket_max = 16; 
};

struct static_config {
    // kevent数组的的尺寸
    static constexpr int kevent_size = 64;
    // socket缓冲区的尺寸
    static constexpr ssize_t buffer_size = 4096;
};

}