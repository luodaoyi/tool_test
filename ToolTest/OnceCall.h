#pragma once
#include <functional>
#include <mutex>
class OnceCall
{
public:
    void Invoke(std::function<void(void)> fn);
private:
    std::once_flag flag_;
};

