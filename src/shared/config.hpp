#pragma once

#include <cstddef>
#include <ctime>
#include <optional>
#include <string>

#include <rfl/toml.hpp>

namespace xkv {

constexpr size_t KB = 1024;
constexpr size_t MB = 1024 * KB;
constexpr size_t GB = 1024 * MB;

// 应用运行时配置
struct app_config {
    // 启用AOF（日志追加的持久化）
    bool enable_aof = false;

    // 端口
    int port = 12463;
    // IO缓冲对象池大小（Windows的IOCP在用）
    size_t io_pool_size = 64;

    // 服务器最大占用内存（占用会比实际偏大，请预留一定空间）
    size_t capacity_max = 4 * GB;

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

    // 从文件中加载配置
    static std::optional<app_config> load();
    // 导出配置，发生错误就抛出
    static void dump(app_config config, std::string file);
};

// 编译时配置
struct static_config {
    // kevent数组的的尺寸
    static constexpr int kevent_size = 64;
    // socket缓冲区的尺寸
    static constexpr size_t buffer_size = 4096;
};

inline std::optional<app_config> app_config::load() {
    if (!std::filesystem::exists("config.toml")) {
        return std::nullopt;
    }

    auto conf = rfl::toml::load<app_config>("config.toml");
    if (!conf.has_value()) {
        throw std::runtime_error(conf.error().what());
    }

    return conf.value();
}

inline void app_config::dump(app_config config, std::string file) {
    auto res = rfl::toml::save(file, config);
    if (!res.has_value()) {
        throw std::runtime_error(res.error().what());
    }
}

// 全局配置对象
inline app_config _config;
inline const app_config &config = _config;

// 是否处于调试环境
inline constexpr bool DEBUG =
#ifdef NDEBUG
    false;
#else
    true;
#endif

} // namespace xkv
