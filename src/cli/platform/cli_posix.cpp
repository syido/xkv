#ifdef _POSIX_VERSION   // PosixAPI

#include "../cli.hpp"

#include <system_error>
#include <unistd.h>

using namespace xkv;
using namespace std;

bool cli::create_process_if_necessary(int, char **) {
    pid_t pid = fork();

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