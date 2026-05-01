#pragma once

#include <cstddef>
namespace xkv {

struct app_config {
    // 触发扩容的因子
    size_t rehash_fractor = 4;
    // 哈希表中桶最大元素数
    size_t bucket_max = 16; 
};

}