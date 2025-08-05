//
//  job.hh
//  rpg
//
//  Created by George Watson on 04/08/2025.
//

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> jobs;
    std::queue<std::function<void()>> priority_jobs;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

public:
    ThreadPool(size_t num_threads): stop(false) {
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
            throw std::runtime_error("enqueue on stopped ThreadPool");
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

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }
};
