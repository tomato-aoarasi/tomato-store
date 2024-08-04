/*
 * @File	  : utils.hpp
 * @Coding	  : utf-8
 * @Author    : Bing
 * @Time      : 2023/05/28 14:11
 * @Introduce : 各种工具
*/

#pragma once

#include <vector>
#include <unistd.h>
#include <thread>
#include <list>
#include <unordered_map>
#include <random>
#include <chrono>
#include <functional>
#include "Poco/UUID.h"
#include "Poco/UUIDGenerator.h"
#include "Poco/DigestEngine.h"
#include "Poco/SHA2Engine.h"
#include "Poco/SHA1Engine.h"
#include "http_util.hpp"
#include "self_exception.hpp"

#ifndef UTILS_HPP
#define UTILS_HPP

namespace self {
	namespace Tools {
		inline bool exec_simple(const char* cmd) {
			FILE* pipe{ popen(cmd, "r") };
			if (!pipe) {
				pclose(pipe);
				return false;
			}
			pclose(pipe);
			return true;
		}

		// 运行shell脚本并获取字符串
		const std::string exec(const char* cmd) {
			FILE* pipe = popen(cmd, "r");
			if (!pipe) {
				pclose(pipe);
				return "ERROR";
			}
			char buffer[128];
			std::string result = "";
			while (!feof(pipe)) {
				if (fgets(buffer, 128, pipe) != NULL)
					result += buffer;
			}
			pclose(pipe);
			return result;
		}

		// 生成随机字符串
		std::string generateRandom(size_t length = 32, const std::string& charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") {
			std::random_device seeder; // 用于生成随机种子    mt19937 gen(rd()); // 以随机种子初始化随机数生成器

			const auto seed{ seeder.entropy() ? seeder() : time(nullptr) };

			std::mt19937 engine{ static_cast<std::mt19937::result_type>(seed) };

			std::uniform_int_distribution<size_t> distribution{ 0, charset.length() - 1 }; // 均匀分布

			std::string randomStr;
			for (size_t i = 0; i < length; i++) {
				randomStr += charset[distribution(engine)]; // 从字符集中随机选择一个字符    
			}
			return randomStr;
		}
	
		/// <summary>
		/// 判断vector<string>是否存在特定值
		/// </summary>
		/// <param name="keys">crow param的列表</param>
		/// <param name="val">是否含有特定字符串</param>
		/// <returns>true为存在,false为不存在</returns>
		inline bool hasParam(const std::vector<std::string>& keys, std::string_view val) {
			return std::find(keys.cbegin(), keys.cend(), val) != keys.cend();
		}

		/// <summary>
		/// 效验request参数
		/// </summary>
		/// <param name="req">req丢进去就完事了</param>
		/// <param name="val">是否存在的字符串</param>
		/// <returns>true返回长度不为0的内容</returns>
		inline bool verifyParam(const crow::request& req, std::string_view val) {
			bool has_value{ hasParam(req.url_params.keys(), val) };
			if (has_value)
			{
				return !std::string(req.url_params.get(val.data())).empty();
			}
			else return false;
		}
	};

	crow::response HandleResponseBody(std::function<std::string(void)> f) {
		crow::response response;
		response.set_header("Content-Type", "application/json");
		try {
			response.write(f());
			response.code = 200;
			return response;
		}
		catch (const self::HTTPException& except) {
			response.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(except.getCode(), except.getMessage(), except.getJson()).dump(2));
			response.code = except.getCode();
		}
		catch (const json::out_of_range& except) {
			response.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(400, except.what()).dump(2));
			response.code = 400;
		}
		catch (const std::exception& except) {
			response.write(HTTPUtil::StatusCodeHandle::getSimpleJsonResult(500, except.what()).dump(2));
			response.code = 500;
		}
		return response;
	};
}

#endif // !UTILS_HPP

