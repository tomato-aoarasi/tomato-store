/*
 * @File	  : config.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/03/05 21:14
 * @Introduce : 配置类(解析yaml)
*/

#pragma once

#include <fstream>
#include <string>
#include <filesystem>
#include <limits>
#include "nlohmann/json.hpp"
#include "yaml-cpp/yaml.h"
#include "fmt/core.h"

#ifndef CONFIG_HPP
#define CONFIG_HPP  

#define OUT_PREFIX_STR "resources"
#define RESOURCE_URI_STR "/resources/<path>"

constexpr const char* BACKUP_FILENAME{ "backup" };

using namespace std::string_literals;
using namespace std::chrono_literals;

crow::SimpleApp app;

namespace std {
	using fmt::format;
	using fmt::format_error;
	using fmt::formatter;
}

using nlohmann::json;
namespace fs = std::filesystem;


namespace self {
	namespace define {
	};
};

namespace Config {
	static YAML::Node config_yaml{ YAML::LoadFile("config.yaml") };
};

namespace Global {
	inline const fs::path resource_path{ Config::config_yaml["server"]["resource-path"].as<std::string>() };
	inline const fs::path out_prefix{ OUT_PREFIX_STR };
	inline std::string auth{ Config::config_yaml["server"]["auth"].as<std::string>() };
	inline const auto cycle{ Config::config_yaml["server"]["cycle"].as<std::time_t>() };
	inline std::unordered_map<std::string, std::time_t> ttl_info;
};

#endif