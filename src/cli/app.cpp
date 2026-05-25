#include "shared/config.hpp"
#include <cli/cli.hpp>
#include <core/core.hpp>

#include <iostream>

using namespace xkv;
using namespace std;

int main(int argc, char **argv) {
    // 查找是否执行其他任务
    if (cli::excute_other_task(argc, argv)) {
        return 0;
    }

    // 加载配置文件
    auto conf = app_config::load();
    if (conf) {
        const_cast<app_config &>(config) = conf.value();
    }

    // 创建cli进程或core进程
    if (cli::create_process_if_necessary(argc, argv)) {
        cout << "hello, xkv!" << '\n';
        cout << (conf ? "已加载配置文件" : "无配置文件") << '\n';
        cout << "服务器将在端口" << config.port << "打开" << '\n';

        core{}.loop();
    }
}
