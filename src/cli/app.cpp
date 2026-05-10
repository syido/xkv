#include "core/config.hpp"
#include "core/init.hpp"
#include <core/core.hpp>
#include <cli/cli.hpp>
#include <iostream>

using namespace xkv;
using namespace std;

int main() {
    cout << "hello, xkv!" << '\n';
    cout << "服务器将在端口" << config.port << "打开" << '\n';

    cli{}.run();
    core{}.loop();
}