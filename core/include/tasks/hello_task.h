#ifndef HELLO_TASK_H
#define HELLO_TASK_H

#include "core/base_task.h"
#include <thread>
#include <atomic>
#include <string>
#include <functional>

class HelloTask : public BaseTask {
private:
    std::string task_name_;
    json params_;
    std::atomic<bool> is_running_ {false};
    std::atomic<bool> stop_requested_ {false};
    std::thread task_thread_;
    std::function<void(const json&)> progress_callback_;
    json current_task_status_json_;

    void run();
    void sendProgressUpdate(int percentage, const std::string& step_message);

public:

    explicit HelloTask(std::string name);
    ~HelloTask() override;

    std::string getTaskName() const override;
    bool isRunning() const override;
    json getStatus() const override;
    void start(const json& params, std::function<void(const json&)> progress_callback) override;
    void stop() override;
};

#endif // HELLO_TASK_H