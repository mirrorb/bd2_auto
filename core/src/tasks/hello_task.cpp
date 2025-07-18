#include "tasks/hello_task.h"
#include <chrono>
#include <opencv2/opencv.hpp>
#include "io/window_handler.h"
#include "io/screenshot.h"
#include "meta/generated_ui_layout.h"
#include "automator/ui_automator.h"

HelloTask::HelloTask(std::string name)
    : ThreadedTask(std::move(name)) 
{
    // 在构造函数中声明式地定义工作流
    registerStep(SETUP_WINDOW,      std::bind(&HelloTask::step_setupWindow, this));
    registerStep(SHOT,              std::bind(&HelloTask::step_shot,        this));
    registerStep(SHOW_IMAGE,        std::bind(&HelloTask::step_showImage,   this));
    registerStep(WAIT_A_BIT,        std::bind(&HelloTask::step_waitABit,    this));
    registerStep(CLEANUP_RESOURCES, std::bind(&HelloTask::step_cleanup,     this));
}

bool HelloTask::step_setupWindow() {
    logger_->info("[Step] 设置游戏窗口...");
    WindowHandler::reset_game_window(1280, 720, 0, 0);
    return true;
}

bool HelloTask::step_shot() {
    logger_->info("[Step] 截取游戏窗口...");
    cv::Mat screen = Screenshot::capture();
    bool success = UIAutomator::try_click(screen, UIElements::UI_DING_YUE);
    // cv::imshow("ScreenShot", screen);
    // cv::waitKey(0);
    return true;
}

bool HelloTask::step_showImage() {
    // logger_->info("[Step] 创建并显示图像...");
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
    // logger_->info("[Step] 模拟长时间工作 (5s)...");
    // for(int i = 0; i < 5; ++i) {
    //     if (stop_requested_.load()) return false; 
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    //     logger_->info("...工作中 " + std::to_string(i+1) + "/5 ...");
    //     cv::waitKey(1); // 保持窗口响应
    // }
    return true;
}

bool HelloTask::step_cleanup() {
    logger_->info("[Step] 清理资源...");
    cv::destroyAllWindows();
    return true;
}
