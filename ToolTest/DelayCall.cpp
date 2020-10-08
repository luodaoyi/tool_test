#include "DelayCall.h"


DelayCall::DelayCall(DWORD ns) :delay_(ns) {}


void DelayCall::Invoke(std::function<void(void)> fn) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto tick = GetTickCount();
    if (last_time_ == 0 || tick - last_time_ > delay_) {
        fn();
        last_time_ = tick;
    }
}