/**
 * @file HeaderGenerator.cpp
 * @brief 一个独立的C++工具，为每个UI元素生成一个独立的编译时常量。
 *
 */

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <cctype> // for isdigit, isalpha
#include <opencv2/opencv.hpp>
#include <windows.h>

const char* UI_METADATA_STRUCT_DEFINITION = R"RAW(
struct UIMetadata {
    const char* name;
    const char* layout_filename;
    cv::Rect location;
};
)RAW";

// 用于校验文件名是否可以作为C++变量名的函数
bool isValidVariableName(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    // 检查第一个字符是否为字母或下划线
    if (!std::isalpha(name[0]) && name[0] != '_') {
        return false;
    }
    // 检查后续所有字符是否为字母、数字或下划线
    for (char const &c : name) {
        if (!std::isalnum(c) && c != '_') {
            return false;
        }
    }
    return true;
}


int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    const std::string layouts_dir = "../../../../assets/layouts";
    const std::string output_header_path = "../../../../include/meta/generated_ui_layout.h";

    std::ofstream ofs(output_header_path);
    if (!ofs.is_open()) return -1;

    std::cout << "启动UI元数据头文件生成器..." << std::endl;

    // 写入头文件头部和结构体定义
    ofs << "#pragma once\n\n";
    ofs << "#include <opencv2/core/types.hpp>\n\n";
    ofs << UI_METADATA_STRUCT_DEFINITION;
    ofs << "\n\nnamespace UIElements {\n\n";

    // 遍历目录并为每个元素生成一个常量定义
    int element_count = 0;
    int skipped_count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(layouts_dir)) {
        const auto& path = entry.path();
        if (entry.is_regular_file() && path.extension() == ".png") {
            // 从路径中获取文件名（不含扩展名）作为变量名基础
            std::string name_str = path.stem().string();

            // 校验文件名
            if (!isValidVariableName(name_str)) {
                std::cerr << "错误: 文件名 '" << name_str << "' 不是一个合法的C++变量名。已跳过。" << std::endl;
                std::cerr << "     (命名规范：只能包含字母、数字和下划线，且不能以数字开头)。" << std::endl;
                skipped_count++;
                continue;
            }

            cv::Mat sparse_img = cv::imread(path.string(), cv::IMREAD_UNCHANGED);
            if(sparse_img.empty() || sparse_img.channels() != 4) continue;
            std::vector<cv::Mat> channels; cv::split(sparse_img, channels);
            cv::Mat mask; cv::threshold(channels[3], mask, 10, 255, cv::THRESH_BINARY);
            std::vector<std::vector<cv::Point>> contours; cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            if (contours.size() == 1) {
                std::string filename_str = path.filename().string();
                cv::Rect loc = cv::boundingRect(contours[0]);

                // 生成常量定义，变量名直接使用文件名
                ofs << "// " << name_str << "\n";
                // 加一个前缀来避免与C++关键字冲突
                ofs << "inline const UIMetadata UI_" << name_str << " = {\n";
                ofs << "    \"" << name_str << "\",\n";
                ofs << "    \"" << filename_str << "\",\n";
                ofs << "    cv::Rect(" << loc.x << ", " << loc.y << ", " << loc.width << ", " << loc.height << ")\n";
                ofs << "};\n\n";
                element_count++;
            }
        }
    }

    // 写入命名空间尾部
    ofs << "} // namespace UIElements\n";

    ofs.close();
    
    std::cout << "\n处理完成！" << std::endl;
    std::cout << "  - 成功生成 " << element_count << " 个UI元素常量。" << std::endl;
    if (skipped_count > 0) {
        std::cout << "  - 因命名不规范跳过了 " << skipped_count << " 个文件。" << std::endl;
    }
    std::cout << "头文件已写入: " << output_header_path << std::endl;
    return 0;
}
