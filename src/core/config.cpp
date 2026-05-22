#include "config.hpp"

#include <optional>
#include <rfl/toml/load.hpp>
#include <rfl/toml/save.hpp>
#include <rfl/toml/write.hpp>

using namespace xkv;
using namespace std;

app_config _config;
const app_config& xkv::config = _config;

optional<app_config> app_config::load() {
    if (!filesystem::exists("config.toml")) {
        return nullopt;
    }

    auto conf = rfl::toml::load<app_config>("config.toml");
    if (!conf.has_value()) {
        throw runtime_error(conf.error().what());
    }

    return conf.value();
}

void app_config::dump(app_config config, std::string file) {
    auto res = rfl::toml::save(file, config);
    if (!res.has_value()) {
        throw runtime_error(res.error().what());
    }
}
