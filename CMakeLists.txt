cmake_minimum_required(VERSION 3.10)
project(bd2_auto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

# 定义通用源文件
set(COMMON_SOURCES
    src/main.cpp
    src/core/service_app.cpp
    src/core/task_manager.cpp
    src/core/json_rpc_handler.cpp
)
file(GLOB TASK_SOURCES CONFIGURE_DEPENDS "src/tasks/*.cpp")

# 合并所有源文件
set(ALL_SOURCES
    ${COMMON_SOURCES}
    ${TASK_SOURCES}
)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# 链接线程库
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
