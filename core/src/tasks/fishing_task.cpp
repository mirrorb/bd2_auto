#include "tasks/fishing_task.h"

#include <windows.h>
#include <algorithm>
#include <string>
#include <opencv2/opencv.hpp>
#include "io/window_handler.h"
#include "basic/exceptions.h"

using namespace cv;

namespace {
struct FishingConfig {
    std::string monitor_name = "BD2 Fisher";
    bool show_monitor = true;

    double rx = 0.395;
    double ry = 0.850;
    double rw = 0.253;
    double rh = 0.051;

    int yellow_padding = 12;
    int blue_padding = -2;
    int hit_cooldown_ms = 600;
    int target_persist = 8;
    int freeze_interval_ms = 120;

    Scalar yellow_low = Scalar(15, 70, 70);
    Scalar yellow_high = Scalar(40, 255, 255);

    Scalar blue_low = Scalar(90, 130, 100);
    Scalar blue_high = Scalar(115, 255, 255);

    Scalar white_low = Scalar(0, 0, 195);
    Scalar white_high = Scalar(180, 50, 255);

    Scalar red_low1 = Scalar(0, 200, 140);
    Scalar red_high1 = Scalar(8, 255, 255);
    Scalar red_low2 = Scalar(172, 200, 140);
    Scalar red_high2 = Scalar(180, 255, 255);

    Scalar green_low = Scalar(35, 50, 40);
    Scalar green_high = Scalar(90, 255, 255);
};

void pressSpace() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_SPACE;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_SPACE;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

FishingConfig loadConfig(const json& params) {
    FishingConfig config;
    const json cfg = params.value("config", json::object());

    config.monitor_name = cfg.value("monitor_name", config.monitor_name);
    config.show_monitor = cfg.value("show_monitor", config.show_monitor);

    const json roi = cfg.value("roi", json::object());
    config.rx = roi.value("x", config.rx);
    config.ry = roi.value("y", config.ry);
    config.rw = roi.value("w", config.rw);
    config.rh = roi.value("h", config.rh);

    const json padding = cfg.value("padding", json::object());
    config.yellow_padding = padding.value("yellow", config.yellow_padding);
    config.blue_padding = padding.value("blue", config.blue_padding);

    config.hit_cooldown_ms = cfg.value("hit_cooldown_ms", config.hit_cooldown_ms);
    config.target_persist = cfg.value("target_persist", config.target_persist);
    config.freeze_interval_ms = cfg.value("freeze_interval_ms", config.freeze_interval_ms);

    return config;
}
} // namespace

FishingTask::FishingTask(std::string name)
    : ThreadedTask(std::move(name)) {
    registerStep(RUN_LOOP, std::bind(&FishingTask::step_runLoop, this));
}

bool FishingTask::step_runLoop() {
    const FishingConfig config = loadConfig(params_);
    int last_x = -1;
    int lock_s = -1;
    int lock_e = -1;
    int lock_timer = 0;
    bool is_blue_target = false;
    ULONGLONG last_hit_time = 0;
    ULONGLONG flash_end = 0;
    ULONGLONG last_freeze_click = 0;

    HDC hdc_mem = NULL;
    HDC hdc_ext = NULL;
    HBITMAP h_bitmap = NULL;
    HBITMAP h_bit_ext = NULL;
    int last_w = 0;
    int last_h = 0;

    Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));

    if (config.show_monitor) {
        namedWindow(config.monitor_name, WINDOW_AUTOSIZE);
    }

    logger_->info("Fishing task started.");

    while (!stop_requested_.load()) {
        ULONGLONG now = GetTickCount64();

        HWND hwnd = NULL;
        try {
            hwnd = WindowHandler::get_game_HWND();
        } catch (const WindowException&) {
            last_x = -1;
            lock_timer = 0;
            Sleep(500);
            continue;
        }

        RECT rc;
        GetClientRect(hwnd, &rc);
        int win_w = rc.right - rc.left;
        int win_h = rc.bottom - rc.top;
        int roi_w = static_cast<int>(win_w * config.rw);
        int roi_h = static_cast<int>(win_h * config.rh);
        int roi_x = static_cast<int>(win_w * config.rx);
        int roi_y = static_cast<int>(win_h * config.ry);
        POINT pt = {0, 0};
        ClientToScreen(hwnd, &pt);

        int ext_h = static_cast<int>(win_h * 0.1);
        int ext_y = roi_y - (ext_h - roi_h) / 2;

        if (!hdc_mem || roi_w != last_w || roi_h != last_h) {
            if (hdc_mem) {
                DeleteDC(hdc_mem);
                DeleteObject(h_bitmap);
                DeleteDC(hdc_ext);
                DeleteObject(h_bit_ext);
            }
            HDC hdc_s = GetDC(NULL);
            hdc_mem = CreateCompatibleDC(hdc_s);
            h_bitmap = CreateCompatibleBitmap(hdc_s, roi_w, roi_h);
            SelectObject(hdc_mem, h_bitmap);
            hdc_ext = CreateCompatibleDC(hdc_s);
            h_bit_ext = CreateCompatibleBitmap(hdc_s, roi_w, ext_h);
            SelectObject(hdc_ext, h_bit_ext);
            ReleaseDC(NULL, hdc_s);
            last_w = roi_w;
            last_h = roi_h;
        }

        HDC hdc_s = GetDC(NULL);
        BitBlt(hdc_mem, 0, 0, roi_w, roi_h, hdc_s, pt.x + roi_x, pt.y + roi_y, SRCCOPY);
        BitBlt(hdc_ext, 0, 0, roi_w, ext_h, hdc_s, pt.x + roi_x, pt.y + ext_y, SRCCOPY);
        ReleaseDC(NULL, hdc_s);

        Mat raw(roi_h, roi_w, CV_8UC4);
        BITMAPINFOHEADER bi = {sizeof(BITMAPINFOHEADER), roi_w, -roi_h, 1, 32, BI_RGB};
        GetDIBits(hdc_mem, h_bitmap, 0, roi_h, raw.data, reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
        Mat bar;
        Mat hsv;
        cvtColor(raw, bar, COLOR_BGRA2BGR);
        cvtColor(bar, hsv, COLOR_BGR2HSV);

        Mat raw_ext(ext_h, roi_w, CV_8UC4);
        BITMAPINFOHEADER bi_e = {sizeof(BITMAPINFOHEADER), roi_w, -ext_h, 1, 32, BI_RGB};
        GetDIBits(hdc_ext, h_bit_ext, 0, ext_h, raw_ext.data, reinterpret_cast<BITMAPINFO*>(&bi_e), DIB_RGB_COLORS);
        Mat bar_ext;
        Mat hsv_ext;
        cvtColor(raw_ext, bar_ext, COLOR_BGRA2BGR);
        cvtColor(bar_ext, hsv_ext, COLOR_BGR2HSV);

        int cur_x = -1;
        bool is_frozen = false;
        bool in_cooldown = (now - last_hit_time < static_cast<ULONGLONG>(config.hit_cooldown_ms));

        Mat r_mask;
        Mat r1;
        Mat r2;
        inRange(hsv, config.red_low1, config.red_high1, r1);
        inRange(hsv, config.red_low2, config.red_high2, r2);
        r_mask = r1 | r2;

        Mat labels;
        Mat stats;
        Mat centroids;
        int num = connectedComponentsWithStats(r_mask, labels, stats, centroids);
        for (int i = 1; i < num; i++) {
            int wx = stats.at<int>(i, CC_STAT_WIDTH);
            if (wx >= 3 && wx <= static_cast<int>(roi_w * 0.12)) {
                int left = stats.at<int>(i, CC_STAT_LEFT);
                Rect srect(std::max(0, left - 5), 0, std::min(roi_w - left + 5, wx + 10), ext_h);
                Mat gmask;
                inRange(hsv_ext(srect), config.green_low, config.green_high, gmask);
                if (countNonZero(gmask) > 3) {
                    continue;
                }
                is_frozen = true;
                break;
            }
        }

        if (is_frozen) {
            if (now - last_freeze_click >= static_cast<ULONGLONG>(config.freeze_interval_ms)) {
                pressSpace();
                last_freeze_click = now;
            }
            last_x = -1;
        } else {
            bool found_yellow = false;
            int best_area = 0;
            Rect best_rect(-1, -1, 0, 0);

            Mat ymask;
            inRange(hsv, config.yellow_low, config.yellow_high, ymask);
            Mat ylabels;
            Mat ystats;
            Mat ycentroids;
            int ny = connectedComponentsWithStats(ymask, ylabels, ystats, ycentroids);
            for (int i = 1; i < ny; i++) {
                int area = ystats.at<int>(i, CC_STAT_AREA);
                if (area > best_area && ystats.at<int>(i, CC_STAT_HEIGHT) > roi_h * 0.25) {
                    best_area = area;
                    best_rect = Rect(ystats.at<int>(i, CC_STAT_LEFT), 0, ystats.at<int>(i, CC_STAT_WIDTH), roi_h);
                    found_yellow = true;
                }
            }

            if (!found_yellow) {
                Mat bmask;
                inRange(hsv, config.blue_low, config.blue_high, bmask);
                morphologyEx(bmask, bmask, MORPH_CLOSE, kernel);
                Mat blabels;
                Mat bstats;
                Mat bcentroids;
                int nb = connectedComponentsWithStats(bmask, blabels, bstats, bcentroids);
                for (int i = 1; i < nb; i++) {
                    int area = bstats.at<int>(i, CC_STAT_AREA);
                    if (area > best_area && bstats.at<int>(i, CC_STAT_HEIGHT) > roi_h * 0.25) {
                        best_area = area;
                        best_rect = Rect(bstats.at<int>(i, CC_STAT_LEFT), 0, bstats.at<int>(i, CC_STAT_WIDTH), roi_h);
                    }
                }
            }

            if (best_area > 0) {
                lock_s = best_rect.x;
                lock_e = best_rect.x + best_rect.width;
                lock_timer = config.target_persist;
                is_blue_target = !found_yellow;
            } else if (lock_timer > 0) {
                lock_timer--;
            } else {
                lock_s = -1;
                lock_e = -1;
                is_blue_target = false;
            }

            Mat wmask;
            inRange(hsv, config.white_low, config.white_high, wmask);
            if (countNonZero(wmask) > 5) {
                std::vector<std::vector<Point>> wcons;
                findContours(wmask, wcons, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
                int best_score = 0;
                for (const auto& cnt : wcons) {
                    Rect br = boundingRect(cnt);
                    if (br.height >= (roi_h * 0.6) && br.width >= 2 && br.width <= 25 && br.height > br.width) {
                        if (br.height > best_score) {
                            best_score = br.height;
                            cur_x = br.x + br.width / 2;
                        }
                    }
                }
            }

            if (!in_cooldown && lock_s != -1 && cur_x != -1) {
                int cur_p = is_blue_target ? config.blue_padding : config.yellow_padding;
                if (lock_s < (roi_w * 0.2)) {
                    cur_p += 5;
                }

                int bl = lock_s - cur_p;
                int br = lock_e + cur_p;
                if (bl > br) {
                    int mid = (lock_s + lock_e) / 2;
                    bl = mid;
                    br = mid;
                }

                bool hit = (cur_x >= bl && cur_x <= br);
                if (!hit && last_x != -1) {
                    int p_min = std::min(last_x, cur_x);
                    int p_max = std::max(last_x, cur_x);
                    if (!(p_max < bl || p_min > br)) {
                        hit = true;
                    }
                }
                if (!hit && last_x == -1 && cur_x >= bl && cur_x <= br) {
                    hit = true;
                }

                if (hit) {
                    pressSpace();
                    last_hit_time = now;
                    flash_end = now + 150;
                    last_x = -1;
                    lock_timer = 0;
                    logger_->info(is_blue_target ? "[BLUE HIT]" : "[YELLOW HIT]");
                }
            }

            if (cur_x != -1) {
                last_x = cur_x;
            }
        }

        if (config.show_monitor) {
            Mat debug_view = bar.clone();
            if (now < flash_end) {
                debug_view += Scalar(0, 80, 0);
            }

            if (is_frozen) {
                putText(debug_view, "ICE", Point(2, roi_h - 5), 1, 0.6, Scalar(0, 0, 255), 1);
            } else if (lock_s != -1) {
                Scalar col = is_blue_target ? Scalar(255, 100, 0) : Scalar(0, 255, 0);
                rectangle(debug_view, Rect(lock_s, 0, lock_e - lock_s, roi_h), col, 1);

                int cur_p = is_blue_target ? config.blue_padding : config.yellow_padding;
                if (lock_s < (roi_w * 0.2)) {
                    cur_p += 5;
                }
                rectangle(debug_view, Rect(lock_s - cur_p, 1, (lock_e + cur_p) - (lock_s - cur_p), roi_h - 2), Scalar(255, 255, 255), 1);

                std::string mode = is_blue_target ? "T:BLUE" : "T:YELL";
                putText(debug_view, mode, Point(2, 10), FONT_HERSHEY_SIMPLEX, 0.4, Scalar(255, 255, 255), 1);

                if (cur_x != -1) {
                    line(debug_view, Point(cur_x, 0), Point(cur_x, roi_h), Scalar(0, 0, 255), 1);
                }
            }

            int disp_w = 600;
            int disp_h = static_cast<int>(disp_w * (static_cast<double>(roi_h) / roi_w));
            Mat resized;
            resize(debug_view, resized, Size(disp_w, disp_h), 0, 0, INTER_NEAREST);
            imshow(config.monitor_name, resized);
            waitKey(1);
        } else {
            Sleep(1);
        }
    }

    if (config.show_monitor) {
        destroyWindow(config.monitor_name);
    }

    if (hdc_mem) {
        DeleteDC(hdc_mem);
        DeleteObject(h_bitmap);
        DeleteDC(hdc_ext);
        DeleteObject(h_bit_ext);
    }

    logger_->info("Fishing task stopped.");
    return true;
}
