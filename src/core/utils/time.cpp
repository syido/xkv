#include "time.hpp"

#include <ctime>

using namespace xkv;

uint xkv::get_time_sec() {
    static time_t base = time(nullptr);
    time_t now = time(nullptr);

    if (now <= base) {
        return 0;
    }

    return static_cast<uint>(now - base);
}
