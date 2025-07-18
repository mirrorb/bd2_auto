#pragma once

#include "base_task.h"
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

class ThreadedTask : public BaseTask {
public:
    using StepId = int;
    using StepAction = std::function<bool()>;

    explicit ThreadedTask(std::string name);
    ~ThreadedTask() override;

    // BaseTask 接口的最终实现
    std::string getTaskName() const final;
    bool isRunning() const final;
    json getStatus() const final;
    void start(const json& params, std::shared_ptr<const Logger> logger) override;
    void stop() final;

protected:
    void registerStep(StepId step_id, StepAction action);
    void updateStatus(const std::string& status_text, int progress = -1);

    // 供子类使用的受保护成员
    std::string task_name_;
    json params_;
    std::shared_ptr<const Logger> logger_;
    std::atomic<bool> stop_requested_{false};

private:
    std::thread task_thread_;
    std::atomic<bool> is_running_{false};
    
    std::vector<StepId> workflow_sequence_;
    std::map<StepId, StepAction> step_actions_;

    mutable std::mutex status_mutex_;
    json task_status_json_;

    void run();
};
