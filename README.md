# XKV

这是一个使用C++开发的高性能的（至少希望上的）key-value数据库，使用IO多路复用存取、模板、单线程与一些自定义数据结构等机制以期更优秀的表现。

简言之，就是<del>抄的</del>致敬的Redis，理论上没有（可能有？）任何的先进之处，除了帮笔者进行一个C++习的学。

## 功能

### 开发中或已部分实现

- 基本架构
  - 基本架构
  - 客户端
- 数据结构
  - 哈希表与其渐进式rehash
- 网络模型
  - IO多路复用的`kqueue`和异步IO的`iocp`（未完善）
  - 简化版的应用层协议
- 读取配置文件
- 持久化
  - 日志持久化AOF
- 测试
  - 压力测试

### 待实现功能

- 测试
  - 模块测试
- 持久化
  - 二进制持久化<del>XDB</del>
- 日志输出与性能窗口
- 订阅事件
- 网络模型
  - IO多路复用的`epoll`版本
- 数据结构
  - 使用第三方内存分配器
  - 哈希表的缩容
  - 自定义的字符串存储

## 使用

虽然本项目似乎不太有使用的需要，但若你只是想体验一下或仅出于好玩（因为笔者也很喜欢捣鼓），可以编译`xkv_cli`可执行文件目标并运行，部分配置项希望在编译前就已编码。

目前仅支持一个哈希表与set、get、remove和cas操作，命令如下请求如下：
```
s key value
g key
c key value new_value
```
s、g、r、c为操作选项，除此之外每个字符串开头需要设置长度，并用`\r`分隔，命令结尾`\r\r`结束，若想通过socket访问，则需要编码成这样：
```
s\r3key\r5value\r\r
```

## 测试

测试代码在`test/`目录下，编译为静态库`xkv_test`。

### 压力测试

链接`xkv_test`静态库并使用以下代码执行压力测试，配置可在`test/benchmark/benchmark.hpp`中查阅。

```cpp
#include <test/benchmark/benchmark.hpp>

using namespace std;
using namespace xkvt::benchmark;

int main() {
    config conf;
    commands cmds = {
        command{.op = operation::set, .size = 10000000, .min_len = 10, .max_len = 20},
        command{.op = operation::get, .size = 10000000, .min_len = 10, .max_len = 20}
    };

    benchmark{conf, cmds}.run();
}
```
