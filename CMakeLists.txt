cmake_minimum_required(VERSION 3.5)
project(BinanceMonitor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 设置 Debug 模式和调试选项
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")


# 添加 include 目录
include_directories(${CMAKE_SOURCE_DIR}/include)
# third-party headers (nlohmann json)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# 源文件（来自 src/ 目录）
set(SOURCES
    src/main.cpp
    src/Worker.cpp
    src/BinanceAPI.cpp
    src/FeishuNotifier.cpp
    src/BinanceAPIManager.cpp
    src/OIAnalyzer.cpp
)

add_executable(monitor ${SOURCES})

# 链接依赖库
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
if(UNIX)
    target_link_libraries(monitor ${CURL_LIBRARIES} pthread)
else()
    target_link_libraries(monitor ${CURL_LIBRARIES})
endif()

# 打印调试信息
message(STATUS "CURL include: ${CURL_INCLUDE_DIRS}")
message(STATUS "CURL libs: ${CURL_LIBRARIES}")
