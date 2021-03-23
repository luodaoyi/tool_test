#include "ThreadPool.h"
#include <assert.h>

namespace Tools {

    void TaskQueue::on_idle() {};


    ThreadPool::ThreadPool(size_t n) : shutdown_(false) {
        while (n) {
            threads_.emplace_back(worker(*this));
            n--;
        }
    }


    void ThreadPool::enqueue(std::function<void()> fn) {
        std::unique_lock<std::mutex> lock(mutex_);
        jobs_.push_back(std::move(fn));
        cond_.notify_one();
    }

    void ThreadPool::shutdown() {
        // Stop all worker threads...
        {
            std::unique_lock<std::mutex> lock(mutex_);
            shutdown_ = true;
        }

        cond_.notify_all();

        // Join...
        for (auto& t : threads_) {
            t.join();
        }
    }


    ThreadPool::worker::worker(ThreadPool& pool) : pool_(pool) {}

    void ThreadPool::worker::operator()() {
        for (;;) {
            std::function<void()> fn;
            {
                std::unique_lock<std::mutex> lock(pool_.mutex_);

                pool_.cond_.wait(
                    lock, [&] { return !pool_.jobs_.empty() || pool_.shutdown_; });

                if (pool_.shutdown_ && pool_.jobs_.empty()) { break; }

                fn = pool_.jobs_.front();
                pool_.jobs_.pop_front();
            }

            assert(true == static_cast<bool>(fn));
            fn();
        }
    }


}