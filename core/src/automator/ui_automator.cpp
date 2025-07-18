#include "automator/ui_automator.h"
#include <meta/generated_ui_layout.h>
#include <basic/base_config.h>
#include <filesystem>
#include "io/mouse_handler.h"
#include "basic/path_util.hpp"

static std::map<std::string, cv::Mat> layout_cache;

bool UIAutomator::verify(const cv::Mat& screen, const UIMetadata& element, double confidence) {
    // 边界检查
    cv::Rect screen_rect(0, 0, screen.cols, screen.rows);
    if ((element.location & screen_rect) != element.location) {
        return false;
    }

    // 从缓存或文件加载模板图像
    cv::Mat template_img;
    auto it = layout_cache.find(element.layout_filename);
    if (it != layout_cache.end()) {
        template_img = it->second;
    } else {
        // 动态构建模板的完整路径
        std::filesystem::path full_template_path = get_executable_directory()
            .append(BaseConfig::ASSETS_LAYOUTS_PATH).append(element.layout_filename);
        template_img = cv::imread(full_template_path.string(), cv::IMREAD_COLOR);
        if (template_img.empty()) {
            // 如果模板文件加载失败，无法进行验证
            return false;
        }
        // 存入缓存
        layout_cache[element.layout_filename] = template_img;
    }

    // 从屏幕截图中裁剪出要比较的区域
    cv::Mat roi = screen(element.location);
    
    // 执行模板匹配
    cv::Mat result;
    // 匹配方法结果范围在-1到1之间
    cv::matchTemplate(roi, template_img, result, cv::TM_CCOEFF_NORMED);
    
    // 获取匹配结果的最高分
    double minVal, maxVal;
    cv::minMaxLoc(result, &minVal, &maxVal);

    // 将最高分与传入的相似度阈值比较
    return maxVal >= confidence;

}

bool UIAutomator::try_click(const cv::Mat& screen, const UIMetadata& element, double confidence) {
    // 先验证，如果成功，再行动。
    if (verify(screen, element, confidence)) {
        MouseHandler::click_in_rect(element.location);
        return true;
    }
    
    // 如果验证失败，直接返回false。
    return false;
}
