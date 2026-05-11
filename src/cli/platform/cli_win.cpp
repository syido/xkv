#ifdef __WINNT

#include "../cli.hpp"

#include <errhandlingapi.h>
#include <libloaderapi.h>
#include <processenv.h>
#include <processthreadsapi.h>
#include <windows.h>


#include <string>

using namespace xkv;
using namespace std;

// 设置utf-8为输出编码
static void set_utf8() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

// 如果参数有--cli，就进入cli循环，否则启动服务
// 对于posix标准的系统，直接通过fork()进入cli{}.loop()
bool cli::create_process_if_necessary(int argc, char **argv) {
    set_utf8();

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--cli") {
            cli{}.loop();
            return false;
        }
    }

    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    wstring cmd = GetCommandLineW();
    cmd += L" --cli";

    CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
    return true;
}

#endif