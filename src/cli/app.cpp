#include "core/config.hpp"
#include "core/init.hpp"
#include <core/core.hpp>
#include <cli/cli.hpp>
#include <iostream>

using namespace xkv;
using namespace std;

int main(int argc, char **argv) {
    if (cli::create_process_if_necessary(argc, argv)) {    // 创建cli进程或core进程
        cout << "hello, xkv!" << '\n';
        cout << "服务器将在端口" << config.port << "打开" << '\n';

        core{}.loop();
    }
}