//
//  job_queue.hpp
//  nice
//
//  Created by George Watson on 04/08/2025.
//

#pragma once

#include <vector>
#include <queue>
#include <deque>
#include <functional>
#include <future>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_set>
#include <shared_mutex>

template<typename T>
class UnorderedSet {
    std::unordered_set<T> _set;
    mutable std::shared_mutex _mutex;

public:
    bool contains(const T& value) const {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        return _set.find(value) != _set.end();
    }

    bool insert(const T& value) {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        return _set.insert(value).second;
    }

    bool erase(const T& value) {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        return _set.erase(value) > 0;
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        return _set.size();
    }

    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(_mutex);
        return _set.empty();
    }

    // Add a clear method for cleanup
    void clear() {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        _set.clear();
    }
};

template<typename T>
class JobQueue {
    std::deque<T> _queue;
    std::deque<T> _priority_queue;
    mutable std::mutex _queue_mutex;
    std::condition_variable _condition;
    std::atomic<bool> _stop{false};
    std::vector<std::thread> _worker_threads;
    std::function<void(T)> _processor;

    void worker_loop() {
        while (true) {
            std::unique_lock<std::mutex> lock(this->_queue_mutex);
            
            // Use timeout to prevent indefinite blocking during shutdown
            if (!this->_condition.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return this->_stop.load() || !this->_priority_queue.empty() || !this->_queue.empty();
            })) {
                // Timeout occurred, check if we should stop
                if (this->_stop.load())
                    return;
                continue;
            }
            
            if (this->_stop.load() && this->_priority_queue.empty() && this->_queue.empty())
                return;
            
            T item;
            bool has_item = false;
            
            // Process priority queue first
            if (!this->_priority_queue.empty()) {
                item = std::move(this->_priority_queue.front());
                this->_priority_queue.pop_front();
                has_item = true;
            } else if (!this->_queue.empty()) {
                item = std::move(this->_queue.front());
                this->_queue.pop_front();
                has_item = true;
            }
            
            lock.unlock(); // Release lock before processing
            if (has_item)
                this->_processor(item);
        }
    }

public:
    explicit JobQueue(std::function<void(T)> processor, size_t num_threads = 1) 
        : _processor(std::move(processor)) {
        _worker_threads.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            _worker_threads.emplace_back([this]() {
                this->worker_loop();
            });
        }
    }

    JobQueue(const JobQueue&) = delete;
    JobQueue& operator=(const JobQueue&) = delete;
    JobQueue(JobQueue&& other) noexcept
        : _queue(std::move(other._queue))
        , _priority_queue(std::move(other._priority_queue))
        , _processor(std::move(other._processor))
        , _stop(other._stop.load()) {
        if (!other._worker_threads.empty())
            _worker_threads = std::move(other._worker_threads);
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

    void push_priority(T item) {
        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            _priority_queue.push_back(std::move(item));
        }
        _condition.notify_one();
    }

    void enqueue(T item) {
        push(std::move(item));
    }

    void enqueue_priority(T item) {
        push_priority(std::move(item));
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(_queue_mutex);
            _stop.store(true);
        }
        _condition.notify_all();
        for (auto& worker : _worker_threads)
            if (worker.joinable())
                worker.join();
        _worker_threads.clear();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        return _queue.empty() && _priority_queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        return _queue.size() + _priority_queue.size();
    }

    size_t worker_count() const {
        return _worker_threads.size();
    }

    size_t pending_jobs() const {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        return _queue.size();
    }

    size_t pending_priority_jobs() const {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        return _priority_queue.size();
    }
};

class GenericJobQueue : public JobQueue<std::function<void()>> {
public:
    explicit GenericJobQueue(size_t num_threads = std::thread::hardware_concurrency()) 
        : JobQueue<std::function<void()>>([](std::function<void()> f) { f(); }, num_threads) {}

    template<class F, class... Args> 
    auto enqueue(bool priority, F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;
        auto job = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> result = job->get_future();
        std::function<void()> task = [job]() { (*job)(); };
        if (priority)
            push_priority(std::move(task));
        else
            push(std::move(task));
        return result;
    }

    template<class F, class... Args> 
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        return enqueue(false, std::forward<F>(f), std::forward<Args>(args)...);
    }

    template<class F, class... Args> 
    auto enqueue_priority(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
        return enqueue(true, std::forward<F>(f), std::forward<Args>(args)...);
    }
};


