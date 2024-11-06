# CMakeList.txt: Csv 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.24)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("Csv")

# 将源代码添加到此项目的可执行文件。
add_executable (Csv "Csv.cpp")

target_include_directories(Csv PUBLIC ${CMAKE_SOURCE_DIR}/include/Csv ${CMAKE_SOURCE_DIR}/include/ConcurrentQueue)

# TODO: 如有需要，请添加测试并安装目标。
