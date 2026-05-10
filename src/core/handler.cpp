#include "handler.hpp"
#include <string>
#include <string_view>
#include <utility>

using namespace xkv;
using namespace std;

constexpr static char op_get = 'g';
constexpr static char op_set = 's';
constexpr static char op_remove = 'r';
constexpr static char op_cas = 'c';

auto handler::handle(const string &request) -> pair<app_result, string> {
    string_view req = request;
    pair<app_result, string> prase_fail = {app_result::parse_failed, string{}};

    // 读一个字符串
    auto prase_string = [&req]() -> string {
        size_t len = 0;

        if (req.empty() || !isdigit(static_cast<unsigned char>(req.front()))) {
            req.remove_prefix(req.size()); // 置空以让外部环境抛出异常
            return {};
        }

        do {
            len = len * 10 + static_cast<size_t>(req.front() - '0');
            req.remove_prefix(1);
        } while (!req.empty() && isdigit(static_cast<unsigned char>(req.front())));

        if (req.size() < len) {
            req.remove_prefix(req.size());
            return {};
        }

        string str{req.substr(0, len)};
        req.remove_prefix(len);

        return str;
    };

    // 读一个'\r'
    auto invalid_spliter = [&req]() -> bool {
        if (req.empty() || req.front() != '\r') {
            return true;
        }

        req.remove_prefix(1);
        return false;
    };

    // == 函数从这里开始 ==
    // 读取操作符
    if (req.empty()) {
        return prase_fail;
    }
    char op = req.front();
    req.remove_prefix(1);

    if (invalid_spliter()) {
        return prase_fail;
    }

    // 读取key
    string key = prase_string();
    if (invalid_spliter()) {
        return prase_fail;
    }

    // GET和REMOVE都只需要一个key参数
    if (op == op_get) {
        auto res = main_store.get(key);
        if (invalid_spliter()) {
            return prase_fail;
        }

        if (!res) {
            return {res.error(), string{}};
        }

        return {app_result::ok, res.value()};
    } else if (op == op_remove) {
        auto res = main_store.remove(key);
        if (invalid_spliter()) {
            return prase_fail;
        } else if (res != app_result::ok) {
            return {res, string{}};
        } else {
            return {app_result::ok, string{}};
        }
    }

    // SET需要第二个参数
    string value1 = prase_string();
    if (invalid_spliter()) {
        return prase_fail;
    }
    if (op == op_set) {
        auto res = main_store.set(key, value1);
        if (invalid_spliter()) {
            return prase_fail;
        } else if (res != app_result::ok) {
            return {res, string{}};
        } else {
            return {app_result::ok, string{}};
        }
    }

    // CAS需要第三个参数
    string value2 = prase_string();
    if (invalid_spliter()) {
        return prase_fail;
    }
    if (op == op_cas) {
        auto res = main_store.compare_and_set(key, value1, value2);
        if (invalid_spliter()) {
            return prase_fail;
        } else if (res != app_result::ok) {
            return {res, string{}};
        } else {
            return {app_result::ok, string{}};
        }
    } else {
        return prase_fail;
    }
}

string handler::make_response(app_result code, string str) {
    static const string Spliter = "\r", Ender = Spliter;
    string size = std::to_string(str.size());

    string buf;
    buf.reserve(2 + Spliter.size() + size.size() + str.size() + Spliter.size() + Ender.size());

    buf.append(to_string(code));
    buf.append(Spliter);
    buf.append(size);
    buf.append(str);
    buf.append(Spliter);
    buf.append(Ender);

    return buf;
}
