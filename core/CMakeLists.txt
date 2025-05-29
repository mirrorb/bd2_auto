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
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(PLATFORM_ENV "gnu")
    else()
        message(FATAL_ERROR "Unsupported C++ compiler on Windows for TARGET_TRIPLE: ${CMAKE_CXX_COMPILER_ID}.")
    endif()
    set(TARGET_TRIPLE "${PLATFORM_ARCH}-${PLATFORM_VENDOR}-${PLATFORM_OS}-${PLATFORM_ENV}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_VENDOR "pc")
    set(PLATFORM_OS "linux")
    set(PLATFORM_ENV "gnu")
    set(TARGET_TRIPLE "${PLATFORM_ARCH}-${PLATFORM_VENDOR}-${PLATFORM_OS}-${PLATFORM_ENV}")
else()
    message(FATAL_ERROR "Unsupported CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}. Only Windows and Linux are configured.")
endif()

# 线程库
find_package(Threads REQUIRED)

# OpenCV库
set(OpenCV_DIR "D:/opencv")
set(OpenCV_STATIC ON)
find_package(Eigen3 REQUIRED)
find_package(OpenEXR REQUIRED)
find_package(Iconv REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${EIGEN3_INCLUDE_DIR})
if(OpenEXR_FOUND AND OpenEXR_INCLUDE_DIRS)
    include_directories(${OpenEXR_INCLUDE_DIRS})
endif()
if(Iconv_FOUND AND Iconv_INCLUDE_DIRS)
    include_directories(${Iconv_INCLUDE_DIRS})
endif()
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

add_executable(${PROJECT_NAME}-${TARGET_TRIPLE} ${ALL_SOURCES})


# 链接线程库
target_link_libraries(${PROJECT_NAME}-${TARGET_TRIPLE} PRIVATE Threads::Threads)

# 链接 OpenCV 库
target_link_libraries(${PROJECT_NAME}-${TARGET_TRIPLE} PRIVATE ${OpenCV_LIBS})

set(EXECUTABLE_OUTPUT_PATH ../bin)


# 测试部分
enable_testing()

add_executable(test_temp 
    tests/sift_test.cpp
    src/cv/point_matcher.cpp
)
target_link_libraries(test_temp PRIVATE ${OpenCV_LIBS})

add_test(NAME test_temp COMMAND test_temp)
