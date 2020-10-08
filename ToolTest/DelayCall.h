#pragma once
#include <Windows.h>
#include <functional>
#include <mutex>
class DelayCall
{
public:
    DelayCall(DWORD ns);
    void Invoke(std::function<void(void)> fn);
private:
    std::mutex mutex_;
    DWORD last_time_ = 0;
    DWORD delay_ = 0;
};
