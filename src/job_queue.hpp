//
//  job_queue.hpp
//  nice
//
//  Created by George Watson on 04/08/2025.
//

#pragma once

#include <deque>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

template<typename T>
class JobQueue {
    std::deque<T> _queue;
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
                
                // Use timeout to prevent indefinite blocking during shutdown
                if (!this->_condition.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return this->_stop.load() || !this->_queue.empty();
                })) {
                    // Timeout occurred, check if we should stop
                    if (this->_stop.load())
                        return;
                    continue;
                }
                
                if (this->_stop.load() && this->_queue.empty())
                    return;
                
                if (this->_queue.empty())
                    continue;
                    
                T item = std::move(this->_queue.front());
                this->_queue.pop_front();
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
            _queue.push_back(std::move(item));
        }
        _condition.notify_one();
    }

    void push_front(T item) {
        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            _queue.push_front(std::move(item));
        }
        _condition.notify_one();
    }

    void enqueue(T item) {
        push(std::move(item));
    }

    void enqueue_priority(T item) {
        push_front(std::move(item));
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            _stop.store(true);
        }
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
