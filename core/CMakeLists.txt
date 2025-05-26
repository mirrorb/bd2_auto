cmake_minimum_required(VERSION 3.10)
project(bd2_auto_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TARGET_TRIPLE "")

if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
    message(FATAL_ERROR "This CMake script is configured for x64 targets only. Current processor: ${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(PLATFORM_ARCH "x86_64") # Common for both

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_VENDOR "pc")
    set(PLATFORM_OS "windows")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(PLATFORM_ENV "msvc")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(PLATFORM_ENV "gnu")
    else()
        message(WARNING "Unsupported C++ compiler on Windows for TARGET_TRIPLE: ${CMAKE_CXX_COMPILER_ID}. Defaulting to 'msvc'.")
        set(PLATFORM_ENV "msvc")
    endif()
    set(TARGET_TRIPLE "${PLATFORM_ARCH}-${PLATFORM_VENDOR}-${PLATFORM_OS}-${PLATFORM_ENV}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_VENDOR "unknown")
    set(PLATFORM_OS "linux")
    set(PLATFORM_ENV "gnu")
    set(TARGET_TRIPLE "${PLATFORM_ARCH}-${PLATFORM_VENDOR}-${PLATFORM_OS}-${PLATFORM_ENV}")
else()
    message(FATAL_ERROR "Unsupported CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}. Only Windows and Linux are configured.")
endif()

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

add_executable(${PROJECT_NAME}-${TARGET_TRIPLE} ${ALL_SOURCES})

# 链接线程库
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME}-${TARGET_TRIPLE} PRIVATE Threads::Threads)

set(EXECUTABLE_OUTPUT_PATH ../bin)
