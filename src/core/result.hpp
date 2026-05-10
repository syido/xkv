#pragma once

#include <string>

namespace xkv {

// 应用的结果类型
enum class app_result : int {

    // 成功
    ok = 0,

    // 错误类型
    out_of_memery, // 内存不足
    not_found,     // 未找到
    constraint,    // 约束错误
    parse_failed,  // 解析错误
    loss_response, // 响应丢失

    unkown = 99,   // 仅用于错误码识别错误的未知错误类型，不会在核心业务中出现

    // 上限限制
    _max_limit
};

static_assert(int(app_result::_max_limit) <= 100, "错误码应为两位数");

extern std::string to_string(app_result code);

} // namespace xkv
