#include "automator/ui_automator.h"
#include <meta/generated_ui.h>
#include <basic/base_config.h>
#include <filesystem>
#include "io/mouse_handler.h"
#include "basic/path_util.hpp"

static std::map<std::string, cv::Mat> layout_cache;
static std::map<std::string, cv::Mat> template_cache;

bool UIAutomator::verify(const cv::Mat& screen, const UILayouts::Metadata& layout, double confidence) {
    // 边界检查
    cv::Rect screen_rect(0, 0, screen.cols, screen.rows);
    if ((layout.location & screen_rect) != layout.location) {
        return false;
    }

    // 从缓存或文件加载模板图像
    cv::Mat layout_img;
    auto it = layout_cache.find(layout.filename);
    if (it != layout_cache.end()) {
        layout_img = it->second;
    } else {
        // 动态构建模板的完整路径
        std::filesystem::path full_template_path = get_executable_directory()
            .append(BaseConfig::ASSETS_LAYOUTS_PATH).append(layout.filename);
        layout_img = cv::imread(full_template_path.string(), cv::IMREAD_COLOR);
        if (layout_img.empty()) {
            // 如果模板文件加载失败，无法进行验证
            return false;
        }
        // 存入缓存
        layout_cache[layout.filename] = layout_img;
    }

    // 从屏幕截图中裁剪出要比较的区域
    cv::Mat roi = screen(layout.location);
    
    // 执行模板匹配
    cv::Mat result;
    // 匹配方法结果范围在-1到1之间
    cv::matchTemplate(roi, layout_img, result, cv::TM_CCOEFF_NORMED);
    
    // 获取匹配结果的最高分
    double minVal, maxVal;
    cv::minMaxLoc(result, &minVal, &maxVal);

    // 将最高分与传入的相似度阈值比较
    return maxVal >= confidence;

}

bool UIAutomator::verify_click(const cv::Mat& screen, const UILayouts::Metadata& layout, double confidence) {
    // 先验证，如果成功，再行动。
    if (verify(screen, layout, confidence)) {
        MouseHandler::click_in_rect(layout.location);
        return true;
    }
    
    // 如果验证失败，直接返回false。
    return false;
}

std::optional<cv::Rect> UIAutomator::find(const cv::Mat& screen, const UITemplates::Metadata& template_, double confidence) {
    // 检查输入图像
    if (screen.empty()) {
        return std::nullopt;
    }

    // 从缓存或文件加载模板图像
    cv::Mat template_img;
    auto it = template_cache.find(template_.filename);
    if (it != template_cache.end()) {
        // 从缓存命中
        template_img = it->second;
    } else {
        // 从文件加载
        std::filesystem::path full_path = get_executable_directory()
            .append(BaseConfig::ASSETS_TEMPLATES_PATH)
            .append(template_.filename);

        template_img = cv::imread(full_path.string(), cv::IMREAD_COLOR);
        if (template_img.empty()) {
            return std::nullopt;
        }
        // 成功加载后，存入缓存
        template_cache[template_.filename] = template_img;
    }

    // 进行边界检查，确保模板不大于屏幕
    if (template_img.cols > screen.cols || template_img.rows > screen.rows) {
        return std::nullopt;
    }

    // 模板匹配
    cv::Mat result;
    // 匹配方法结果范围在-1到1之间
    cv::matchTemplate(screen, template_img, result, cv::TM_CCOEFF_NORMED);

    // 获取匹配结果的最高分及其位置
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

    // 最高分与传入的相似度阈值比较
    if (maxVal >= confidence) {
        return cv::Rect(maxLoc.x, maxLoc.y, template_img.cols, template_img.rows);
    }

    // 未找到满足置信度的匹配项
    return std::nullopt;

}

bool UIAutomator::find_click(const cv::Mat& screen, const UITemplates::Metadata& template_, double confidence) {
    // 定位模板
    auto found_location = find(screen, template_, confidence);

    if (found_location) {
        // 解包 cv::Rect
        const cv::Rect& rect = *found_location;

        // 执行点击操作
        MouseHandler::click_in_rect(rect);

        return true;
    } else {
        return false;
    }
}
