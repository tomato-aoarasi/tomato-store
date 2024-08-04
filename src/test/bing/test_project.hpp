#pragma once

#include <iostream>
#include <fmt/core.h>
#include <fstream>
#include <memory>
#include <exception>
#include <stdexcept>
#include <utility>
#include <filesystem>
#include <locale>
#include <codecvt>
#include <string>
#include <span>
#include "crow.h"

#ifndef TEST_PROJECT_HPP
#define TEST_PROJECT_HPP  
//#define WARNING_CONTENT  

class TestProject final {
public:
	inline static void test(void) {
		
	};
private:
	inline static std::chrono::system_clock::time_point start, end;

	inline static void StartWatch(void) {
		std::cout << "\033[42mstart watch\033[0m\n";
		start = std::chrono::high_resolution_clock::now();
	};

	inline static void StopWatch(void) {
		end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = end - start;
		std::cout << "\033[42mtime consumed " << elapsed.count() << " ms\033[0m\n";
		std::cout.flush();
	};
};

#endif