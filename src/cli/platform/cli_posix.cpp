#if defined(__APPLE__) || defined(__linux__) // PosixAPI

#include "../cli.hpp"

#include <system_error>
#include <unistd.h>

using namespace xkv;
using namespace std;

bool cli::create_process_if_necessary(int argc, char **argv) {
    pid_t pid = fork();

    // 对于posix，现在也支持--cli启动客户端
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--cli" || arg == "-c") {
            cli{}.loop();
            return false;
        }
    }

    if (pid == 0) {
        cli{}.loop();
        return false;
    } else if (pid == -1) {
        throw std::system_error{errno, std::generic_category(), "创建posix进程出错"};
    } else {
        return true;
    }
}

#endif