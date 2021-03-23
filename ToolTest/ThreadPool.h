#pragma once
#include <functional>
#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <list>

namespace Tools {

    class TaskQueue {
    public:
        TaskQueue() = default;
        virtual ~TaskQueue() = default;

        virtual void enqueue(std::function<void()> fn) = 0;
        virtual void shutdown() = 0;

        virtual void on_idle();
    };

    class ThreadPool : public TaskQueue {
    public:
        explicit ThreadPool(size_t n);

        ThreadPool(const ThreadPool&) = delete;
        ~ThreadPool() override = default;

        void enqueue(std::function<void()> fn) override;

        void shutdown() override;

    private:
        struct worker {
            explicit worker(ThreadPool& pool);

            void operator()();

            ThreadPool& pool_;
        };
        friend struct worker;

        std::vector<std::thread> threads_;
        std::list<std::function<void()>> jobs_;

        bool shutdown_;

        std::condition_variable cond_;
        std::mutex mutex_;
    };

}