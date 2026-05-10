#include "../cli.hpp"

#include <system_error>
#include <unistd.h>

using namespace xkv;
using namespace std;

void cli::run() {
    pid_t pid = fork();

    if (pid == 0) {
        cli{}.loop();
        _exit(0);
    } else if (pid == -1) {
        throw std::system_error{errno, std::generic_category(), "创建posix进程出错"};
    }
}
