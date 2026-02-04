#include "tasks/hello_task.h"
#include <chrono>
#include <thread>
#include <opencv2/opencv.hpp>
#include "io/window_handler.h"
#include "io/screenshot.h"
#include "meta/generated_ui.h"
#include "automator/ui_automator.h"

HelloTask::HelloTask(std::string name)
    : ThreadedTask(std::move(name)) 
{
    // 定义该任务的工作流步骤
    registerStep(SETUP_WINDOW,      std::bind(&HelloTask::step_setupWindow, this));
    registerStep(SHOT,              std::bind(&HelloTask::step_shot,        this));
    registerStep(SHOW_IMAGE,        std::bind(&HelloTask::step_showImage,   this));
    registerStep(WAIT_A_BIT,        std::bind(&HelloTask::step_waitABit,    this));
    registerStep(CLEANUP_RESOURCES, std::bind(&HelloTask::step_cleanup,     this));
}

bool HelloTask::step_setupWindow() {
    logger_->info("[Step] 设置游戏窗口...");
    const bool reset_window = params_.value("reset_window", true);
    const int width = params_.value("window_width", 1280);
    const int height = params_.value("window_height", 720);
    if (reset_window) {
        WindowHandler::reset_game_window(width, height, 0, 0);
    }
    return true;
}

bool HelloTask::step_shot() {
    logger_->info("[Step] 截取游戏窗口...");
    cv::Mat screen = Screenshot::capture();
    bool success = UIAutomator::verify_click(screen, UILayouts::UI_DING_YUE);
    UIAutomator::drag(UILayouts::UI_RI_ZHI.location, UILayouts::UI_DING_YUE.location);
    // cv::imshow("ScreenShot", screen);
    // cv::waitKey(0);
    return true;
}

bool HelloTask::step_showImage() {
    // logger_->info("[Step] 创建并显示测试图像...");
    // cv::Mat black_image = cv::Mat::zeros(cv::Size(400, 300), CV_8UC3);
    // if (black_image.empty()) {
    //     logger_->error("无法创建黑色图像");
    //     return false;
    // }
    // cv::putText(black_image, "Workflow OK!", cv::Point(50, 150),
    //             cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(0, 255, 255), 2);
    // cv::imshow("Workflow Test", black_image);
    // cv::waitKey(0);
    return true;
}

bool HelloTask::step_waitABit() {
    const int wait_seconds = params_.value("wait_seconds", 0);
    if (wait_seconds <= 0) {
        return true;
    }
    logger_->info("[Step] 模拟工作 (" + std::to_string(wait_seconds) + "秒)...");
    for (int i = 0; i < wait_seconds; ++i) {
        if (stop_requested_.load()) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        cv::waitKey(1);
    }
    return true;
}

bool HelloTask::step_cleanup() {
    logger_->info("[Step] 清理资源...");
    cv::destroyAllWindows();
    return true;
}
