# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.22)

project ("tomato-store")

set(PROJECT_ROOT_NODE ${CMAKE_CURRENT_SOURCE_DIR})

# 将指定目录文件所有复制到二进制文件目录
file(GLOB allCopyFiles  "${CMAKE_CURRENT_SOURCE_DIR}/resource/*")
file(COPY ${allCopyFiles} DESTINATION ${PROJECT_ROOT_NODE}/bin)

set(CATKIN_TOPLEVEL TRUE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)

# 输出项目信息
message(STATUS ${PROJECT_NAME} ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR})

# 三方库开始

# 协程库
find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(Crow REQUIRED)

# 指定最终的可执行文件的位置
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_ROOT_NODE}/bin)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
# 将源代码添加到此项目的可执行文件。
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)
add_executable (${PROJECT_NAME} ${SRC_LIST})

# 库链接
target_link_libraries(${PROJECT_NAME} PUBLIC
nlohmann_json::nlohmann_json fmt::fmt yaml-cpp 
Crow::Crow spdlog::spdlog)

message(G:${CMAKE_C_COMPILER_VERSION},GXX:${CMAKE_CXX_COMPILER_VERSION},CXX STANDARD:${CMAKE_CXX_STANDARD})
message(final)