#pragma once

#include <windows.h>

class ThreadInputAttacher {
public:
    ThreadInputAttacher() {
        // 获取前台窗口的线程ID和当前线程ID
        DWORD foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
        myThreadId_ = GetCurrentThreadId();
        targetThreadId_ = foregroundThreadId;

        // 仅在需要时附加
        if (myThreadId_ != targetThreadId_) {
            AttachThreadInput(myThreadId_, targetThreadId_, TRUE);
            isAttached_ = true;
        }
    }

    ~ThreadInputAttacher() {
        if (isAttached_) {
            AttachThreadInput(myThreadId_, targetThreadId_, FALSE);
        }
    }

    ThreadInputAttacher(const ThreadInputAttacher&) = delete;
    ThreadInputAttacher& operator=(const ThreadInputAttacher&) = delete;

private:
    DWORD myThreadId_ = 0;
    DWORD targetThreadId_ = 0;
    bool isAttached_ = false;
};
