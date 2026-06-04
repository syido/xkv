#include "handler.hpp"
#include <shared/config.hpp>
#include <shared/result.hpp>

#include <string>
#include <string_view>

using namespace xkv;
using namespace std;

constexpr static char op_get = 'g';
constexpr static char op_set = 's';
constexpr static char op_remove = 'r';
constexpr static char op_cas = 'c';
constexpr static char op_info = 'i';
constexpr static char op_reset = 'R';

auto handler::handle(const string &request) -> handler::result {
    string_view req = request;
    handler::result prase_fail = {
        .code = app_result::parse_failed,
        .response = string_view{},
    };

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

    // INFO和RESET_DEBUG不需要参数
    if (op == op_info || op == op_reset) {
        if (invalid_spliter()) {
            return prase_fail;
        } else if (op == op_info) {
            return {app_result::ok, main_store.info()};
        } else if (op == op_reset) {
            // 调试环节通过，否则拒绝
            if constexpr (DEBUG) {
                main_store.reset_debug();
                return {app_result::ok, };
            } else {
                return {app_result::debug_off, };
            }
        }
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
            return {
                res.error(),
            };
        }

        return {app_result::ok, string_view{res.value()}};
    } else if (op == op_remove) {
        if (invalid_spliter()) {
            return prase_fail;
        } else {
            auto res = main_store.remove(key);
            return {
                res,
            };
        }
    }

    // SET需要第二个参数
    string value1 = prase_string();
    if (invalid_spliter()) {
        return prase_fail;
    }
    if (op == op_set) {
        if (invalid_spliter()) {
            return prase_fail;
        } else {
            auto res = main_store.set(key, value1);
            return {
                res,
            };
        }
    }

    // CAS需要第三个参数
    string value2 = prase_string();
    if (invalid_spliter()) {
        return prase_fail;
    }
    if (op == op_cas) {
        if (invalid_spliter()) {
            return prase_fail;
        } else {
            auto res = main_store.compare_and_set(key, value1, value2);
            return {
                res,
            };
        }
    } else {
        return prase_fail;
    }
}

string handler::make_response(const handler::result &result) {
    auto str = result.response;
    static const string Spliter = "\r", Ender = Spliter;
    string size = std::to_string(str.size());

    string buf;
    buf.reserve(2 + Spliter.size() + size.size() + str.size() + Spliter.size() + Ender.size());

    buf.append(to_string(result.code));
    buf.append(Spliter);
    buf.append(size);
    buf.append(str);
    buf.append(Spliter);
    buf.append(Ender);

    return buf;
}
