#include "tasks/hello_task.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

#include "io/backend.h"
#include "io/mouse_handler.h"
#include "io/screenshot.h"
#include "io/window_handler.h"

namespace {

struct CaptureBenchmarkStats {
    int success_count = 0;
    int failure_count = 0;
    double avg_ms = 0.0;
};

cv::Rect clamp_rect_to_image(const cv::Rect& rect, const cv::Size& size) {
    const cv::Rect bounds(0, 0, size.width, size.height);
    return rect & bounds;
}

cv::Mat annotate_capture(const cv::Mat& image, const std::string& label, const cv::Rect& click_rect) {
    cv::Mat annotated = image.clone();
    if (annotated.empty()) {
        return annotated;
    }

    const cv::Rect safe_rect = clamp_rect_to_image(click_rect, annotated.size());
    if (safe_rect.width > 0 && safe_rect.height > 0) {
        cv::rectangle(annotated, safe_rect, cv::Scalar(0, 255, 255), 2);
    }

    cv::rectangle(annotated, cv::Rect(0, 0, annotated.cols, 38), cv::Scalar(25, 25, 25), cv::FILLED);
    cv::putText(
        annotated,
        label,
        cv::Point(12, 26),
        cv::FONT_HERSHEY_SIMPLEX,
        0.7,
        cv::Scalar(255, 255, 255),
        2,
        cv::LINE_AA
    );

    return annotated;
}

cv::Mat build_preview(const cv::Mat& before, const cv::Mat& after, const cv::Rect& click_rect) {
    if (before.empty() || after.empty()) {
        return {};
    }

    cv::Mat left = annotate_capture(before, "Before click", click_rect);
    cv::Mat right = annotate_capture(after, "After click", click_rect);

    if (left.size() != right.size()) {
        cv::resize(right, right, left.size(), 0.0, 0.0, cv::INTER_LINEAR);
    }

    cv::Mat combined;
    cv::hconcat(left, right, combined);
    return combined;
}

std::string format_benchmark_line(const std::string& name, const CaptureBenchmarkStats& stats) {
    std::ostringstream oss;
    oss << name << ": success=" << stats.success_count
        << ", failed=" << stats.failure_count
        << ", avg=" << std::fixed << std::setprecision(2) << stats.avg_ms << " ms";
    return oss.str();
}

} // namespace

HelloTask::HelloTask(std::string name)
    : ThreadedTask(std::move(name)) {
    registerStep(SETUP_WINDOW, std::bind(&HelloTask::step_setupWindow, this));
    registerStep(WAIT_A_BIT, std::bind(&HelloTask::step_waitABit, this));
    registerStep(SHOT_BEFORE, std::bind(&HelloTask::step_shotBefore, this));
    registerStep(TEST_INPUT, std::bind(&HelloTask::step_testInput, this));
    registerStep(SHOT_AFTER, std::bind(&HelloTask::step_shotAfter, this));
    registerStep(BENCHMARK_CAPTURE, std::bind(&HelloTask::step_benchmarkCapture, this));
    registerStep(SHOW_IMAGE, std::bind(&HelloTask::step_showImage, this));
    registerStep(CLEANUP_RESOURCES, std::bind(&HelloTask::step_cleanup, this));
}

bool HelloTask::step_setupWindow() {
    logger_->info("[Step] Checking game window...");

    const auto io_backend = IOBackend::from_string(params_.value("io_backend", std::string("window_message")));
    const bool reset_window = params_.value("reset_window", false);
    const int width = params_.value("window_width", 1280);
    const int height = params_.value("window_height", 720);

    WindowHandler::find_game_window();
    if (reset_window) {
        WindowHandler::normalize_game_window(width, height, 0, 0, io_backend);
    }
    WindowHandler::sync_game_window_mode(io_backend);

    const int click_x = params_.value("click_x", 20);
    const int click_y = params_.value("click_y", 116);
    const int click_w = params_.value("click_w", 120);
    const int click_h = params_.value("click_h", 28);
    click_rect_ = cv::Rect(click_x, click_y, click_w, click_h);

    logger_->info(std::string("[Step] Active backend: ") + IOBackend::to_string(io_backend));
    logger_->info(
        "[Step] Test click rect: x=" + std::to_string(click_rect_.x) +
        ", y=" + std::to_string(click_rect_.y) +
        ", w=" + std::to_string(click_rect_.width) +
        ", h=" + std::to_string(click_rect_.height)
    );
    logger_->info("[Step] Cover the game window now if you want to test occlusion/background behavior.");
    return true;
}

bool HelloTask::step_waitABit() {
    const int wait_seconds = 10;
    logger_->info("[Step] Waiting 10 seconds before test...");

    for (int i = 0; i < wait_seconds; ++i) {
        if (stop_requested_.load()) {
            return false;
        }
        logger_->info("[Step] Countdown: " + std::to_string(wait_seconds - i) + "s");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        cv::waitKey(1);
    }
    return true;
}

bool HelloTask::step_shotBefore() {
    const auto io_backend = IOBackend::from_string(params_.value("io_backend", std::string("window_message")));
    logger_->info(std::string("[Step] Capturing BEFORE image with backend: ") + IOBackend::to_string(io_backend));

    captured_before_ = Screenshot::capture_with_backend(io_backend);
    if (captured_before_.empty()) {
        logger_->error("Before image is empty.");
        return false;
    }
    return true;
}

bool HelloTask::step_testInput() {
    const auto io_backend = IOBackend::from_string(params_.value("io_backend", std::string("window_message")));
    logger_->info("[Step] Sending background click to the test region...");

    MouseHandler::click_in_rect_with_backend(click_rect_, io_backend, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(params_.value("post_click_wait_ms", 500)));
    return true;
}

bool HelloTask::step_shotAfter() {
    const auto io_backend = IOBackend::from_string(params_.value("io_backend", std::string("window_message")));
    logger_->info(std::string("[Step] Capturing AFTER image with backend: ") + IOBackend::to_string(io_backend));

    captured_after_ = Screenshot::capture_with_backend(io_backend);
    if (captured_after_.empty()) {
        logger_->error("After image is empty.");
        return false;
    }

    preview_image_ = build_preview(captured_before_, captured_after_, click_rect_);
    if (preview_image_.empty()) {
        logger_->error("Failed to build the preview image.");
        return false;
    }
    return true;
}

bool HelloTask::step_benchmarkCapture() {
    const bool benchmark_capture = params_.value("benchmark_capture", true);
    if (!benchmark_capture) {
        benchmark_summary_.clear();
        logger_->info("[Benchmark] Skipped.");
        return true;
    }

    const int iterations = (std::max)(1, params_.value("benchmark_iterations", 50));
    logger_->info("[Step] Running capture benchmark...");
    logger_->warn("[Step] The win32 capture benchmark may raise the game window to the foreground.");

    const auto run_benchmark = [&](IOBackend::Mode backend) {
        CaptureBenchmarkStats stats;
        double total_ms = 0.0;

        for (int i = 0; i < iterations; ++i) {
            if (stop_requested_.load()) {
                break;
            }

            const auto started_at = std::chrono::steady_clock::now();
            try {
                cv::Mat image = Screenshot::capture_with_backend(backend);
                if (!image.empty()) {
                    const auto elapsed = std::chrono::steady_clock::now() - started_at;
                    total_ms += std::chrono::duration<double, std::milli>(elapsed).count();
                    ++stats.success_count;
                } else {
                    ++stats.failure_count;
                }
            } catch (const std::exception&) {
                ++stats.failure_count;
            }
        }

        if (stats.success_count > 0) {
            stats.avg_ms = total_ms / static_cast<double>(stats.success_count);
        }
        return stats;
    };

    const CaptureBenchmarkStats message_stats = run_benchmark(IOBackend::Mode::WindowMessage);
    const CaptureBenchmarkStats win32_stats = run_benchmark(IOBackend::Mode::Win32);

    const std::string message_line = format_benchmark_line("window_message", message_stats);
    const std::string win32_line = format_benchmark_line("win32", win32_stats);
    benchmark_summary_ = message_line + "\n" + win32_line;

    logger_->info("[Benchmark] " + message_line);
    logger_->info("[Benchmark] " + win32_line);
    return true;
}

bool HelloTask::step_showImage() {
    if (preview_image_.empty()) {
        logger_->error("No preview image available.");
        return false;
    }

    const std::string preview_window_name = params_.value("preview_window_name", std::string("Message Backend Test"));
    logger_->info("[Step] Showing comparison image. Close the window or press any key to continue.");
    if (!benchmark_summary_.empty()) {
        logger_->info("[Benchmark] " + benchmark_summary_);
    }

    cv::namedWindow(preview_window_name, cv::WINDOW_AUTOSIZE);
    cv::imshow(preview_window_name, preview_image_);
    cv::waitKey(0);
    return true;
}

bool HelloTask::step_cleanup() {
    logger_->info("[Step] Cleaning up preview resources...");
    captured_before_.release();
    captured_after_.release();
    preview_image_.release();
    benchmark_summary_.clear();
    cv::destroyAllWindows();
    return true;
}
