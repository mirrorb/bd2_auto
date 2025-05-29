#include "tasks/hello_task.h"
#include <opencv2/opencv.hpp>

HelloTask::HelloTask(std::string name) : task_name_(std::move(name)) {
    current_task_status_json_["task_name"] = task_name_; // 初始化状态JSON
    current_task_status_json_["status_detail"] = "未开始";
}

HelloTask::~HelloTask() {
    stop(); // 请求停止任务
    if (task_thread_.joinable()) {
        task_thread_.join(); // 等待任务线程结束
    }
}

std::string HelloTask::getTaskName() const {
    return task_name_;
}

bool HelloTask::isRunning() const {
    return is_running_.load(); // 原子操作读取
}

json HelloTask::getStatus() const {
    json status;
    status["task_name"] = task_name_;
    status["is_running"] = is_running_.load();
    status["params"] = params_; // 可以选择性地返回任务启动参数
    status["details"] = current_task_status_json_; // 返回内部维护的状态
    return status;
}

void HelloTask::start(const json& params, std::function<void(const json&)> sender) {
    if (is_running_.load()) {
        if (sender) { // 检查回调是否有效
            json error_msg;
            error_msg["type"] = "log";
            error_msg["level"] = "warn";
            error_msg["message"] = "任务 '" + task_name_ + "' 已在运行中，无法重复启动。";
            sender(error_msg);
        }
        return;
    }
    params_ = params; // 保存参数
    sender_ = sender; // 保存回调
    stop_requested_ = false; // 重置停止请求标志
    is_running_ = true; // 设置运行状态
    current_task_status_json_["status_detail"] = "正在初始化...";

    // 创建并启动任务线程
    task_thread_ = std::thread(&HelloTask::run, this);
}

void HelloTask::stop() {
    stop_requested_ = true; // 设置停止请求标志
    // 实际的停止发生在run()方法内部检查此标志
    current_task_status_json_["status_detail"] = "正在请求停止...";
    
}

// 任务线程的执行函数
void HelloTask::run() {
    current_task_status_json_["status_detail"] = "已启动";
    sendProgressUpdate(0, "任务启动");

    // 创建一个简单的黑色图像并显示它
    cv::Mat black_image = cv::Mat::zeros(cv::Size(400, 300), CV_8UC3);
    if (black_image.empty()) {
        std::cerr << "错误: 无法创建黑色图像!" << std::endl;
    }
    cv::putText(black_image, "OpenCV OK!", cv::Point(50, 150), 
                cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0, 255, 0), 3);
    cv::imshow("OpenCV Test", black_image);
    cv::waitKey(0); // 等待按键
    cv::destroyAllWindows();

    is_running_ = false; // 任务正常完成，更新运行状态
    current_task_status_json_["status_detail"] = "已完成";
    sendProgressUpdate(100, "任务完成");

    // 任务完成后可以发送一个特定的完成事件
    if (sender_) {
        json completion_event;
        completion_event["type"] = "event";
        completion_event["event_name"] = "task_completed";
        completion_event["task_name"] = task_name_;
        completion_event["data"]["message"] = "任务 '" + task_name_ + "' 成功执行完毕。";
        sender_(completion_event);
    }
}

// 辅助函数，用于发送进度更新
void HelloTask::sendProgressUpdate(int percentage, const std::string& step_message) {
    if (sender_) {
        json j_progress;
        j_progress["type"] = "progress";
        j_progress["task_name"] = task_name_;
        j_progress["current_step"] = step_message;
        j_progress["percentage"] = percentage;
        sender_(j_progress);
    }
}