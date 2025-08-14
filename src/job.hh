//
//  job.hh
//  rpg
//
//  Created by George Watson on 04/08/2025.
//

#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <future>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>

template<typename T>
class JobQueue {
    std::queue<T> _queue;
    mutable std::mutex _queue_mutex;
    std::condition_variable _condition;
    std::atomic<bool> _stop{false};
    std::thread _worker_thread;
    std::function<void(T)> _processor;

public:
    explicit JobQueue(std::function<void(T)> processor) : _processor(std::move(processor)) {
        _worker_thread = std::thread([this]() {
            while (true) {
                std::unique_lock<std::mutex> lock(this->_queue_mutex);
                this->_condition.wait(lock, [this] {
                    return this->_stop.load() || !this->_queue.empty();
                });
                
                if (this->_stop.load() && this->_queue.empty())
                    return;
                    
                T item = std::move(this->_queue.front());
                this->_queue.pop();
                lock.unlock(); // Release lock before processing
                
                this->_processor(item);
            }
        });
    }

    JobQueue(const JobQueue&) = delete;
    JobQueue& operator=(const JobQueue&) = delete;
    JobQueue(JobQueue&& other) noexcept
        : _queue(std::move(other._queue))
        , _processor(std::move(other._processor))
        , _stop(other._stop.load()) {
        if (other._worker_thread.joinable())
            _worker_thread = std::move(other._worker_thread);
    }

    ~JobQueue() {
        stop();
    }

    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            _queue.push(std::move(item));
        }
        _condition.notify_one();
    }

    void enqueue(T item) {
        push(std::move(item));
    }

    void stop() {
        _stop.store(true);
        _condition.notify_all();
        if (_worker_thread.joinable())
            _worker_thread.join();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        return _queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        return _queue.size();
    }
};

class JobPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> jobs;
    std::queue<std::function<void()>> priority_jobs;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

public:
    JobPool(size_t num_threads): stop(false) {
        for (size_t i = 0; i < num_threads; ++i)
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> job;
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->priority_jobs.empty() || !this->jobs.empty();
                    });
                    if (this->stop && this->priority_jobs.empty() && this->jobs.empty())
                        return;
                    if (!this->priority_jobs.empty()) {
                        job = std::move(this->priority_jobs.front());
                        this->priority_jobs.pop();
                    } else {
                        job = std::move(this->jobs.front());
                        this->jobs.pop();
                    }
                    job();
                }
            });
    }

    template<class F, class... Args> auto enqueue(bool priority, F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;
        auto job = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> result = job->get_future();
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop)
            throw std::runtime_error("enqueue on stopped JobPool");
        if (priority)
            priority_jobs.emplace([job](){(*job)();});
        else
            jobs.emplace([job](){(*job)();});
        condition.notify_one();
        return result;
    }

    size_t pending_jobs() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        return jobs.size();
    }

    size_t worker_count() const {
        return workers.size();
    }

    ~JobPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }
};
