#pragma once

#include "basic/threaded_task.h"

class FishingTask : public ThreadedTask {
private:
    enum FishingTaskSteps : StepId {
        RUN_LOOP
    };

    bool step_runLoop();

public:
    explicit FishingTask(std::string name);
    ~FishingTask() override = default;
};
