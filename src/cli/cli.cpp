#include "cli.hpp"
#include <shared/config.hpp>
#include <shared/result.hpp>

#include <asio.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace xkv;
using namespace std;

// 解析结果码，返回执行成功/失败与对应信息
static pair<bool, string> prase_result(string_view result) {
    static auto to_app_result = [](string_view result) -> app_result {
        if (result.size() != 2 || result[0] < '0' || result[0] > '9' || result[1] < '0' || result[1] > '9') {
            return app_result::unkown;
        }

        int code = result[1] - '0' + 10 * (result[0] - '0');
        return static_cast<app_result>(code);
    };

    auto res_code = to_app_result(result);
    switch (res_code) {
    case app_result::ok:
        return {true, "成功"};
    case app_result::updated:
        return {true, "更新成功"};
    case app_result::out_of_memery:
        return {false, "内存不足"};
    case app_result::not_found:
        return {false, "未找到"};
    case app_result::constraint:
        return {false, "约束错误"};
    case app_result::parse_failed:
        return {false, "解析错误"};
    case app_result::loss_response:
        return {false, "响应丢失"};
    case app_result::unkown:
        return {false, "???客户端错误"};

    default:
        return {false, "非法错误码"};
    }
}

static string make_request(const string &line) {
    istringstream input{line};

    vector<string> tokens;
    string token;
    while (input >> token) {
        tokens.push_back(std::move(token));
    }

    if (tokens.empty()) {
        return {};
    }

    string request;
    request.append(tokens[0]);
    request.push_back('\r');

    for (size_t i = 1; i < tokens.size(); i++) {
        request.append(to_string(tokens[i].size()));
        request.append(tokens[i]);
        request.push_back('\r');
    }

    request.push_back('\r');
    return request;
}

static string send_request(const string &request) {
    asio::io_context context;
    asio::ip::tcp::socket socket{context};
    asio::ip::tcp::endpoint endpoint{
        asio::ip::make_address("127.0.0.1"),
        static_cast<unsigned short>(config.port),
    };

    socket.connect(endpoint);
    asio::write(socket, asio::buffer(request));

    array<char, static_config::buffer_size> buffer{};
    asio::error_code error;
    size_t size = socket.read_some(asio::buffer(buffer), error);

    if (error == asio::error::eof) {
        return {};
    }
    if (error) {
        throw runtime_error{"读取core回复失败"};
    }

    return string{buffer.data(), size};
}

static string parse_response_body(string_view response) {
    if (response.size() <= 3 || response[2] != '\r') {
        return {};
    }

    response.remove_prefix(3);

    size_t len = 0;
    while (!response.empty() && response.front() >= '0' && response.front() <= '9') {
        len = len * 10 + static_cast<size_t>(response.front() - '0');
        response.remove_prefix(1);
    }

    if (response.size() < len) {
        return {};
    }

    return string{response.substr(0, len)};
}

void cli::start_input() {
    string line;
    cout << "> " << flush;
    while (getline(cin, line)) {
        if (line.empty()) {
            cout << "> " << flush;
            continue;
        }

        try {
            string request = make_request(line);
            if (request.empty()) {
                continue;
            }

            string response = send_request(request);
            if (response.size() < 2) {
                cout << "<响应丢失>" << endl;
                continue;
            }

            string_view code_view{response.data(), 2};
            auto [ok, message] = prase_result(code_view);
            string body = parse_response_body(response);

            if (ok) {
                if (!body.empty()) {
                    cout << body << endl;
                }
            } else {
                cout << '<' << message << '>' << endl;
            }
        } catch (const exception &err) {
            cout << '<' << err.what() << '>' << endl;
        }

        cout << "> " << flush;
    }
}

void cli::loop() {
    while (true) {
        try {
            start_input();
        } catch (std::exception &e) {
            std::cout << "! cli发生错误: " << e.what() << std::endl;
        }
    }
}

static bool dump_default_conf_if_necessary(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "template-config" || arg == "tc") {
            app_config::dump(app_config{}, "./template_config.toml");
            cout << "已导出配置文件模版";
            return true;
        }
    }

    return false;
}

bool xkv::cli::excute_other_task(int argc, char **argv) {
    return dump_default_conf_if_necessary(argc, argv);
}
