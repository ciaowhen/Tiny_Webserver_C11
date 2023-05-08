//
// Created by user on 2023/4/25.
//

#ifndef ADVANCECODE_THREADPOOL_H
#define ADVANCECODE_THREADPOOL_H

#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <cassert>

class ThreadPool
{
public:
    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    explicit ThreadPool(size_t thread_count = 8):m_pool(std::make_shared<Pool>())
    {
        assert(thread_count > 0);
        for(int i = 0; i < thread_count; ++i)
        {
            std::thread([pool = m_pool]
            {
                std::unique_lock<std::mutex> locker(pool->mtx);
                while (true)
                {
                    if(pool->is_close)
                    {
                        break;
                    }

                    if(!pool->tasks.empty())
                    {
                        auto do_task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        do_task();
                        locker.lock();
                    }
                    else
                    {
                        pool->cond.wait(locker);
                    }
                }
            }).detach();
        }
    }

    ~ThreadPool()
    {
        if(static_cast<bool>(m_pool))
        {
            std::lock_guard<std::mutex> locker(m_pool->mtx);
            m_pool->is_close = true;
        }

        m_pool->cond.notify_all();
    }

    template<class T> void AddTask(T &&task)
    {
        {
            std::lock_guard<std::mutex> locker(m_pool->mtx);
            m_pool->tasks.emplace(std::forward<T>(task));
        }

        m_pool->cond.notify_one();
    }

private:
    struct Pool
    {
        std::mutex mtx;
        std::condition_variable cond;
        bool is_close;
        std::queue<std::function<void()>> tasks;
    };

    std::shared_ptr<Pool> m_pool;
};

#endif //ADVANCECODE_THREADPOOL_H
