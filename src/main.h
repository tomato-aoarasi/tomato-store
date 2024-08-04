#pragma once

// 设置1为开启跨域访问(想要性能问题的话建议关闭,使用反向代理)
#include <chrono> 
#include <filesystem>
#include <functional>
#include <stack>
#include <csignal> 
#include <unistd.h>
#include "crow/app.h"
#include "crow/multipart.h"

#include "spdlog/spdlog.h"
#include "fmt/format.h"

namespace std {
    using fmt::format;
    using fmt::format_error;
    using fmt::formatter;
}
#include "configuration/config.hpp"
#include "common/log_system.hpp"
#include "common/utils.hpp"
#include "common/self_exception.hpp"
#include "common/http_util.hpp"
#include "self/reusable.hpp"

#ifndef MAIN_H
#define MAIN_H

using namespace std::literals::string_view_literals;
using namespace std::chrono_literals;

void SignalHandler(int signum) {
    LogSystem::logInfo(fmt::format("Interrupt signal ({}) received.", signum));
    // 清理和关闭  
    // 执行程序退出前的必要操作
    {
        json save;

        for (const auto& pair : Global::ttl_info) {
            save.emplace_back(pair);
        }

        if (not save.is_null()) {
            LogSystem::logInfo("正在对关闭进程转存储");
            std::ofstream file(BACKUP_FILENAME, std::ios::out);
            file << save.dump();
            // 关闭文件
            file.close();
            LogSystem::logInfo("backup存储完成");
        } else {
            if (fs::exists(BACKUP_FILENAME)) { // 检查文件是否存在
                fs::remove(BACKUP_FILENAME); // 删除文件
            }
        }
    }

    // 退出程序  
    app.stop();
}


void TTLHandle() {
    while (true) {
        std::stack<decltype(Global::ttl_info)::key_type> filepath_stack;
        std::time_t current_timestamp;
        std::time(&current_timestamp);
        for (const auto& [file_path, ttl] : Global::ttl_info) {
            if (current_timestamp > ttl) {
                filepath_stack.push(file_path);
            }
        }

        while (!filepath_stack.empty()) {
            const auto& file_path{ filepath_stack.top() };

            if (fs::exists(file_path)) { // 检查文件是否存在
                fs::remove(file_path); // 删除文件
            }
            Global::ttl_info.erase(file_path);
            LogSystem::logInfo(fmt::format("已清理: {}", file_path.c_str()));
            filepath_stack.pop();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(Global::cycle));
    }
}

inline std::jthread TTLHandleThread(TTLHandle);

//初始化
void init(void) {
    LogSystem::initialized();
    {
        // ctrl + C关闭处理
        signal(SIGINT, SignalHandler);

        // 异常终止信号
        signal(SIGABRT, SignalHandler);

        // 段错误信号
        signal(SIGSEGV, SignalHandler);

        // core dump处理
        signal(SIGQUIT, SignalHandler);

        // kill -15 处理
        signal(SIGTERM, SignalHandler);
    }

    // 如果存在文件则读取存储
    if (fs::exists(BACKUP_FILENAME)) {
        LogSystem::logInfo("发现backup,正常尝试读取数据");
        std::ifstream backup_if;
        backup_if.open(BACKUP_FILENAME, std::ios::in);

        std::string backup_str;

        backup_if >> backup_str;

        for (auto& data : json::parse(backup_str)) {
            fs::path filepath{ data.at(0).get<std::string>() };
            std::time_t timestamp{ data.at(1).get<std::time_t>() };
            Global::ttl_info[filepath] = timestamp;
        }

        LogSystem::logInfo("已读取backup存储");
    }

    // 周期处理
    TTLHandleThread.detach();
}

inline void checkAuthorization(const crow::request& req){
    auto authentication{ req.get_header_value("Authorization")};
    if ("Bearer " + Global::auth != authentication){
        throw self::HTTPException(401);
    }
}

// 启动项
inline void start(void) {
    init();

    auto port{ Config::config_yaml["server"]["port"].as<std::uint16_t>() };
    auto host{ Config::config_yaml["server"]["host"].as<std::string>() };
    auto server_name{ Config::config_yaml["server"]["server-name"].as<std::string>() };
    auto timeout{ Config::config_yaml["server"]["time-out"].as<std::uint8_t>() };

    //define your endpoint at the root directory
    CROW_ROUTE(app, "/")([](){
        auto page{ crow::mustache::load_text("index.html") };
        return page;
    });

    //Authorization: Bearer 
    CROW_ROUTE(app, "/upload")
        .methods(crow::HTTPMethod::Post)
        ([](const crow::request& req) {
        return self::HandleResponseBody([&] {
            checkAuthorization(req);
            if (req.body.empty()) { throw self::HTTPException(400, "form-data is empty"s); }

            crow::multipart::message file_message(req);

            const auto& part_map{ file_message.part_map };
            
            std::time_t ttl{ 0 };

            if (part_map.contains("x-ttl")) {
                ttl = std::stoll(part_map.find("x-ttl")->second.body);
            }
            if(not part_map.contains("x-key"))throw self::HTTPException(400, "'x-key' require"s);

            const std::string& filename{ part_map.find("x-key")->second.body };

            if(not part_map.contains("x-scope"))throw self::HTTPException(400, "'x-scope' require"s);

            fs::path relative_dir{ part_map.find("x-scope")->second.body };

            // 对ttl处理相关
            std::string filepath;
            {
                fs::path dirPath{ Global::out_prefix / relative_dir };

                if (not fs::exists(dirPath)) {
                    // 如果目录不存在，则创建它  
                    fs::create_directories(dirPath);
                }

                filepath = dirPath / filename;
            }

            if(not part_map.contains("file"))throw self::HTTPException(400, "'file' require"s);

            auto file{ part_map.find("file") };
            {
                const auto& part_name = file->first;
                const auto& part_value = file->second;
                CROW_LOG_DEBUG << "Part: " << part_name;
                auto headers_it = part_value.headers.find("Content-Disposition");
                if (headers_it == part_value.headers.end())
                {
                    throw self::HTTPException(400, "No Content-Disposition found"s);
                }
                auto params_it = headers_it->second.params.find("filename");
                if (params_it == headers_it->second.params.end())
                {
                    throw self::HTTPException(400, std::format("Part with name \"{}\" should have a file", filename));
                }
                // const std::string outfile_name = params_it->second;

                for (const auto& part_header : part_value.headers)
                {
                    const auto& part_header_name = part_header.first;
                    const auto& part_header_val = part_header.second;
                    CROW_LOG_DEBUG << "Header: " << part_header_name << '=' << part_header_val.value;
                    for (const auto& param : part_header_val.params)
                    {
                        const auto& param_key = param.first;
                        const auto& param_val = param.second;
                        CROW_LOG_DEBUG << " Param: " << param_key << ',' << param_val;
                    }
                }

                std::ofstream outfile(filepath, std::ios::binary);

                if (outfile.is_open()) {
                    outfile.write(part_value.body.c_str(), part_value.body.size()); // 将二进制数据写入文件  
                    outfile.close();
                }
                else {
                    outfile.close();
                    throw self::HTTPException(500, "There is an issue with the output path"s);
                }

                CROW_LOG_INFO << " Contents written to " << filepath;
            }

            if (ttl > 0) {
                std::time_t current_timestamp;
                std::time(&current_timestamp);
                auto expire_timestamp{ ttl + current_timestamp };
                Global::ttl_info[filepath] = expire_timestamp;
                LogSystem::logInfo(fmt::format("已存储: {} | Expire Timestamp: {}", filepath.c_str(), expire_timestamp));
            }
            else
            {
                LogSystem::logInfo(fmt::format("已存储: {}", filepath.c_str()));
            }

            return "{\"code\": 200, \"message\": \"success\"}";
            });
            });

    CROW_ROUTE(app, RESOURCE_URI_STR).methods(crow::HTTPMethod::Get)([](const crow::request& req, const std::string& file) {
        crow::response response;

        auto path{ Global::out_prefix / file };
        LogSystem::logInfo(fmt::format("访问文件: {}", path.c_str()));

        response.set_static_file_info(path);
        return response;
        });

    // 图标
    CROW_ROUTE(app, "/favicon.ico").methods(crow::HTTPMethod::Get)([&]() {
        crow::response response;

        auto path{ Global::resource_path / "favicon.ico"s};

        if (!std::filesystem::exists(path)) {
            response.set_header("Content-Type", "application/json");
            response.code = 404;
            response.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(404).dump(2));
            return response;
        }

        // 获取当前时间
        auto now{ std::chrono::system_clock::now() };

        // 计算七天后的时间
        auto seven_days_later{ now + std::chrono::hours(24 * 7) };

        // 将时间转换为时间戳（秒数）
        auto timestamp{ std::chrono::system_clock::to_time_t(seven_days_later) };

        // 将时间戳转换为 Crow 框架中的 Expires 数值
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&timestamp), "%a, %d %b %Y %H:%M:%S GMT");
        std::string expires{ ss.str() };
        response.set_static_file_info(path);
        response.set_header("Cache-Control", "public");
        response.set_header("Expires", expires);
        return response;
    });

    app.signal_clear();
    app.server_name(server_name);
    app.timeout(timeout);
    app.bindaddr(host);
    app.port(port);

    app.multithreaded().run_async();

    return;
}

#endif