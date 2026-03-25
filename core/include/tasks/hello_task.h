#pragma once

#include <string>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>

#include "basic/threaded_task.h"

class HelloTask : public ThreadedTask {
private:
    cv::Mat captured_before_;
    cv::Mat captured_after_;
    cv::Mat preview_image_;
    cv::Rect click_rect_;
    std::string benchmark_summary_;

    enum HelloTaskSteps : StepId {
        SETUP_WINDOW,
        WAIT_A_BIT,
        SHOT_BEFORE,
        TEST_INPUT,
        SHOT_AFTER,
        BENCHMARK_CAPTURE,
        SHOW_IMAGE,
        CLEANUP_RESOURCES
    };

    bool step_setupWindow();
    bool step_waitABit();
    bool step_shotBefore();
    bool step_testInput();
    bool step_shotAfter();
    bool step_benchmarkCapture();
    bool step_showImage();
    bool step_cleanup();

public:
    explicit HelloTask(std::string name);
    ~HelloTask() override = default;
};
