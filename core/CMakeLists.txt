cmake_minimum_required(VERSION 3.15)
project(bd2_auto_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# 输出目标平台
if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND 
         CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND 
         CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$"))
    message(FATAL_ERROR "Unsupported target platform. This project requires Windows 64-bit with the MSVC C++ compiler.")
endif()

if(MSVC)
    add_compile_options(/utf-8 /MP)
endif()

# 线程库
find_package(Threads REQUIRED)

# OpenCV库
set(OpenCV_DIR "D:/opencv")
find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc calib3d videoio imgcodecs features2d flann)

# 包含头文件目录
include_directories(${OpenCV_INCLUDE_DIRS} include)

# 主程序
set(MAIN_ENTER_SOURCE src/main.cpp)
file(GLOB COMMON_SOURCES CONFIGURE_DEPENDS "src/basic/*.cpp")
file(GLOB CV_SOURCES CONFIGURE_DEPENDS "src/cv/*.cpp")
file(GLOB IO_SOURCES CONFIGURE_DEPENDS "src/io/*.cpp")
file(GLOB TASK_SOURCES CONFIGURE_DEPENDS "src/tasks/*.cpp")
file(GLOB AUTOMATOR_SOURCES CONFIGURE_DEPENDS "src/automator/*.cpp")
set(ALL_SOURCES ${MAIN_ENTER_SOURCE} ${COMMON_SOURCES} ${CV_SOURCES} ${IO_SOURCES} ${AUTOMATOR_SOURCES} ${TASK_SOURCES})
add_executable(${PROJECT_NAME} ${ALL_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${OpenCV_LIBS})
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/core"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/core"
)

# UI元素生成器
set(UI_ELEMENT_GENERATOR ui_element_generator)
add_executable(${UI_ELEMENT_GENERATOR} src/dev/ui_element_generator.cpp)
target_include_directories(${UI_ELEMENT_GENERATOR} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${UI_ELEMENT_GENERATOR} PRIVATE ${OpenCV_LIBS})
set_target_properties(${UI_ELEMENT_GENERATOR}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/dev"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/dev"
)

# 测试程序
enable_testing()
set(TEST_TEMP test_temp)
add_executable(${TEST_TEMP} tests/sift_test.cpp src/cv/point_matcher.cpp)
target_include_directories(${TEST_TEMP} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${TEST_TEMP} PRIVATE ${OpenCV_LIBS})
set_target_properties(${TEST_TEMP}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/test"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/test"
)
add_test(NAME ${TEST_TEMP} COMMAND ${TEST_TEMP})

# opencv动态库拷贝
function(copy_linked_opencv_dlls target)
    # 仅在Windows上执行
    if(NOT WIN32 OR NOT OpenCV_FOUND)
        return()
    endif()

    # 遍历所有通过 find_package 找到的OpenCV库 (OpenCV::core, OpenCV::highgui, etc.)
    foreach(lib ${OpenCV_LIBS})
        # 为每一个库添加一个后期构建命令
        add_custom_command(
            TARGET ${target} POST_BUILD
            # 使用生成器表达式 $<TARGET_FILE:...> 来获取DLL的实际路径
            # 它会在构建时自动选择Debug或Release版本的DLL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${lib}>"
                "$<TARGET_FILE_DIR:${target}>"
            # 在构建日志中打印信息，方便查看
            COMMENT "Copying dependency: $<TARGET_FILE_NAME:${lib}>"
        )
    endforeach()
endfunction()

copy_linked_opencv_dlls(${PROJECT_NAME})
copy_linked_opencv_dlls(${TEST_TEMP})
copy_linked_opencv_dlls(${UI_ELEMENT_GENERATOR})

# assets文件夹拷贝
function(copy_assets target)
    set(ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)
    if(EXISTS ${ASSETS_DIR})
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory
                "$<TARGET_FILE_DIR:${target}>/assets"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:${target}>/assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${ASSETS_DIR}
                "$<TARGET_FILE_DIR:${target}>/assets"
            COMMENT "Copying assets to $<TARGET_FILE_DIR:${target}>"
        )
    endif()
endfunction()
copy_assets(${PROJECT_NAME})
