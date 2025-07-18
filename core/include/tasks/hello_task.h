#pragma once

#include "basic/threaded_task.h"

class HelloTask : public ThreadedTask {
private:
    // 使用 enum class 定义步骤
    enum HelloTaskSteps : StepId {
        SETUP_WINDOW,
        SHOT,
        SHOW_IMAGE,
        WAIT_A_BIT,
        CLEANUP_RESOURCES
    };

    // 每个步骤的具体实现
    bool step_setupWindow();
    bool step_shot();
    bool step_showImage();
    bool step_waitABit();
    bool step_cleanup();

public:
    explicit HelloTask(std::string name);
    ~HelloTask() override = default;
};
