#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <cctype>
#include <vector>
#include <utility>

#include <opencv2/opencv.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

const char* LAYOUT_METADATA_STRUCT_DEFINITION = R"RAW(
// 包含固定位置信息的Layout元数据
struct Metadata {
    const char* name;
    const char* filename;
    cv::Rect location;
};
)RAW";

const char* TEMPLATE_METADATA_STRUCT_DEFINITION = R"RAW(
// 仅用于模板搜索的Tempalte元数据
struct Metadata {
    const char* name;
    const char* filename;
};
)RAW";

/**
 * @brief 校验一个字符串是否可以作为C++变量名。
 * @param name 要校验的字符串。
 * @return 如果有效则返回true，否则返回false。
 */
bool isValidVariableName(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    if (!std::isalpha(static_cast<unsigned char>(name[0])) && name[0] != '_') {
        return false;
    }
    for (char const &c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return true;
}

/**
 * @brief 处理 Layouts 目录，生成 UILayouts 命名空间及其内容。
 */
std::pair<int, int> generateLayouts(std::ofstream& ofs, const std::filesystem::path& layouts_dir) {
    int success_count = 0;
    int skipped_count = 0;
    
    ofs << "\nnamespace UILayouts {\n\n";
    ofs << LAYOUT_METADATA_STRUCT_DEFINITION << "\n\n";

    if (!std::filesystem::exists(layouts_dir)) {
        std::cerr << "警告: Layouts 目录不存在: " << layouts_dir.string() << std::endl;
        ofs << "} // namespace UILayouts\n";
        return {0, 0};
    }

    for (const auto& entry : std::filesystem::directory_iterator(layouts_dir)) {
        const auto& path = entry.path();
        if (!entry.is_regular_file() || path.extension() != ".png") continue;

        std::string name_str = path.stem().string();
        if (!isValidVariableName(name_str)) {
            std::cerr << "错误 [Layout]: 文件名 '" << name_str << "' 不是一个合法的C++变量名。已跳过。\n";
            skipped_count++;
            continue;
        }

        // 图片校验
        cv::Mat sparse_img = cv::imread(path.string(), cv::IMREAD_UNCHANGED);
        if (sparse_img.empty()) {
            std::cerr << "警告 [Layout]: 文件 '" << path.filename().string() << "' 无法被读取。已跳过。\n";
            skipped_count++;
            continue;
        }

        const int REQUIRED_WIDTH = 1280;
        const int REQUIRED_HEIGHT = 720;
        // 尺寸校验
        if (sparse_img.cols != REQUIRED_WIDTH || sparse_img.rows != REQUIRED_HEIGHT) {
            std::cerr << "错误 [Layout]: 文件 '" << path.filename().string() 
                      << "' 的尺寸为 " << sparse_img.cols << "x" << sparse_img.rows
                      << "，不符合标准尺寸 " << REQUIRED_WIDTH << "x" << REQUIRED_HEIGHT
                      << "。已跳过。\n";
            skipped_count++;
            continue;
        }

        // Alpha 通道校验
        if (sparse_img.channels() != 4) {
            std::cerr << "警告 [Layout]: 文件 '" << path.filename().string() << "' 不是有效的4通道PNG。已跳过。\n";
            skipped_count++;
            continue;
        }

        std::vector<cv::Mat> channels;
        cv::split(sparse_img, channels);
        cv::Mat mask;
        cv::threshold(channels[3], mask, 10, 255, cv::THRESH_BINARY);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 轮廓校验
        if (contours.size() == 1) {
            std::string filename_str = path.filename().string();
            cv::Rect loc = cv::boundingRect(contours[0]);

            ofs << "// " << name_str << "\n";
            ofs << "inline const Metadata UI_" << name_str << " = {\n";
            ofs << "    \"" << name_str << "\",\n";
            ofs << "    \"" << filename_str << "\",\n";
            ofs << "    cv::Rect(" << loc.x << ", " << loc.y << ", " << loc.width << ", " << loc.height << ")\n";
            ofs << "};\n\n";
            success_count++;
        } else {
            std::cerr << "警告 [Layout]: 在 '" << path.filename().string() << "' 中找到 " << contours.size() << " 个轮廓 (需要1个)。已跳过。\n";
            skipped_count++;
        }
    }
    ofs << "} // namespace UILayouts\n";
    return {success_count, skipped_count};
}


/**
 * @brief 处理 Templates 目录，生成 UITemplates 命名空间及其内容。
 */
std::pair<int, int> generateTemplates(std::ofstream& ofs, const std::filesystem::path& templates_dir) {
    int success_count = 0;
    int skipped_count = 0;

    ofs << "\nnamespace UITemplates {\n\n";
    ofs << TEMPLATE_METADATA_STRUCT_DEFINITION << "\n\n";

    if (!std::filesystem::exists(templates_dir)) {
        std::cerr << "警告: Templates 目录不存在: " << templates_dir.string() << std::endl;
        ofs << "} // namespace UITemplates\n";
        return {0, 0};
    }

    for (const auto& entry : std::filesystem::directory_iterator(templates_dir)) {
        const auto& path = entry.path();
        if (!entry.is_regular_file() || path.extension() != ".png") continue;

        std::string name_str = path.stem().string();
        if (!isValidVariableName(name_str)) {
            std::cerr << "错误 [Template]: 文件名 '" << name_str << "' 不是一个合法的C++变量名。已跳过。\n";
            skipped_count++;
            continue;
        }

        // 图片校验
        cv::Mat template_img = cv::imread(path.string(), cv::IMREAD_COLOR);
        if (template_img.empty()) {
            std::cerr << "警告 [Template]: 文件 '" << path.filename().string() << "' 无法被读取。已跳过。\n";
            skipped_count++;
            continue;
        }
        
        std::string filename_str = path.filename().string();

        ofs << "// " << name_str << "\n";
        ofs << "inline const Metadata UI_" << name_str << " = {\n";
        ofs << "    \"" << name_str << "\",\n";
        ofs << "    \"" << filename_str << "\"\n";
        ofs << "};\n\n";
        success_count++;
    }
    ofs << "} // namespace UITemplates\n";
    return {success_count, skipped_count};
}

int main() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #endif

    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    // 路径配置
    const std::filesystem::path base_assets_dir = "../../../../assets";
    const std::filesystem::path layouts_dir = base_assets_dir / "layouts";
    const std::filesystem::path templates_dir = base_assets_dir / "templates";
    const std::filesystem::path output_header_path = "../../../../include/meta/generated_ui.h";

    // 确保输出目录存在
    if (const auto parent_path = output_header_path.parent_path(); !parent_path.empty()) {
        std::filesystem::create_directories(parent_path);
    }
    
    std::ofstream ofs(output_header_path);
    if (!ofs.is_open()) {
        std::cerr << "错误：无法打开输出文件 " << output_header_path.string() << std::endl;
        return 1;
    }

    std::cout << "启动UI元数据头文件生成器..." << std::endl;

    // 写入头文件头部
    ofs << "#pragma once\n\n";
    ofs << "#include <opencv2/core/types.hpp>\n";

    // 生成metadata数据
    auto layout_result = generateLayouts(ofs, layouts_dir);
    auto template_result = generateTemplates(ofs, templates_dir);
    
    ofs.close();
    
    // 报告总结
    int total_success = layout_result.first + template_result.first;
    int total_skipped = layout_result.second + template_result.second;

    std::cout << "\n处理完成！" << std::endl;
    std::cout << "  - 成功生成 " << total_success << " 个UI元素常量 ("
              << layout_result.first << " 个 Layout, "
              << template_result.first << " 个 Template)。" << std::endl;
    if (total_skipped > 0) {
        std::cout << "  - 共跳过了 " << total_skipped << " 个文件。" << std::endl;
    }
    std::cout << "头文件已写入: " << std::filesystem::absolute(output_header_path).string() << std::endl;
    
    return 0;
}
