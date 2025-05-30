cmake_minimum_required(VERSION 3.10)
project(bd2_auto_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TARGET_TRIPLE "")

# 平台校验
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
    message(FATAL_ERROR "This CMake script is configured for x64 targets only. Current processor: ${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(PLATFORM_ARCH "x86_64") # Common for both

# 输出目标平台
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_VENDOR "pc")
    set(PLATFORM_OS "windows")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(PLATFORM_ENV "msvc")
    else()
        message(FATAL_ERROR "Unsupported C++ compiler. Please use MSVC.")
    endif()
    set(TARGET_TRIPLE "${PLATFORM_ARCH}-${PLATFORM_VENDOR}-${PLATFORM_OS}-${PLATFORM_ENV}")
else()
    message(FATAL_ERROR "Unsupported CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}. Only Windows MSVC configured.")
endif()

if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/MP)

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MTd")
endif()

# 线程库
find_package(Threads REQUIRED)

# OpenCV库
set(OpenCV_DIR "D:/opencv")
set(OpenCV_STATIC ON)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(include)

# 定义通用源文件
set(ENTER_SOURCE src/main.cpp)
file(GLOB COMMON_SOURCES CONFIGURE_DEPENDS "src/basic/*.cpp")
file(GLOB CV_SOURCES CONFIGURE_DEPENDS "src/cv/*.cpp")
file(GLOB TASK_SOURCES CONFIGURE_DEPENDS "src/tasks/*.cpp")

# 合并所有源文件
set(ALL_SOURCES
    ${ENTER_SOURCE}
    ${COMMON_SOURCES}
    ${CV_SOURCES}
    ${TASK_SOURCES}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG    ../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  ../bin)

add_executable(${PROJECT_NAME}-${TARGET_TRIPLE} ${ALL_SOURCES})

# 链接线程库
target_link_libraries(${PROJECT_NAME}-${TARGET_TRIPLE} PRIVATE Threads::Threads)

# 链接 OpenCV 库
target_link_libraries(${PROJECT_NAME}-${TARGET_TRIPLE} PRIVATE ${OpenCV_LIBS})

# 测试部分
enable_testing()

add_executable(test_temp 
    tests/sift_test.cpp
    src/cv/point_matcher.cpp
)
target_link_libraries(test_temp PRIVATE ${OpenCV_LIBS})

add_test(NAME test_temp COMMAND test_temp)

