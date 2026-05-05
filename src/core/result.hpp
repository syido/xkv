namespace xkv {

// 应用的结果类型
enum class app_result : int {
    ok = 0, // 成功

    // 错误类型
    out_of_memery, // 内存不足
    not_found      // 未找到
};

}

