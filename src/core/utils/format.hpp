#pragma once

#include <cstddef>
#include <format>
#include <string>

inline std::string capacity_format(std::size_t capacity) {
    static constexpr const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    static constexpr std::size_t unit_count = sizeof(units) / sizeof(units[0]);

    double value = static_cast<double>(capacity);
    std::size_t unit_index = 0;

    while (value >= 1024.0 && unit_index + 1 < unit_count) {
        value /= 1024.0;
        ++unit_index;
    }

    if (unit_index == 0) {
        return std::format("{}{}", capacity, units[unit_index]);
    }

    return std::format("{:.2f}{}", value, units[unit_index]);
}
