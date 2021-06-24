/*
 * Multithread library
 *
 * MIT License
 *
 * Copyright (c) 2021 Alex Dolzhenkov
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/** @file mt/thread_pool.hpp
 *
 * @brief The main aim of this library is implementation standard function using
 * multithread algorithms. Also it allows to implement your self multithread
 * algorithms more easy.
 *
 * @note This library required modern C++11 or higher
 *
 * @author Alex Dolzhenkov
 */

#ifndef MT_THREAD_POOL_HPP
#define MT_THREAD_POOL_HPP

#include <thread>       // for std::thread
#include <mutex>        // for std::mutex
#include <vector>       // for std::vector
#include <queue>        // for std::queue
#include <future>       // for std::packeged_task
#include <functional>   // for std::bind
#include <type_traits>  // for std::result_of
#include <atomic>       // for std::atomic

namespace mt {

// See alternative option: https://stackoverflow.com/questions/53014805/add-a-stdpackaged-task-to-an-existing-thread
class thread_pool {
public:
    thread_pool(thread_pool&) = delete;
    thread_pool& operator=(thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete; // TODO
    thread_pool& operator=(thread_pool&&) = delete; // TODO
    thread_pool(size_t threads_amount = std::thread::hardware_concurrency()) {
        // Initialise all worker threads
        for (size_t i = 0; i < threads_amount; i++) {
            threads.push_back(std::async(std::launch::async, [&]{worker();}));
        }
    }
    ~thread_pool() {
        // We should first wait for all threads to finish, as some of them may create new tasks
        wait();

        // Send signal to stop processing
        shutdown_request = true;

        // Wake up all workers to let them finish processing
        queue_cv.notify_all();

        // Wait until all threads will be finished
    }
#ifdef MT_POOL_RET_SUPPORT // slower
    // Note: you also need to replace "std::queue<std::function<void()>> task_queue;" with "std::queue<std::packaged_task<void()>> task_queue;".
    template<class Function, class... Args, class R = typename std::result_of<Function(Args...)>::type>
    std::future<R> push(Function&& func, Args&&... args) {
        std::packaged_task<R()> task(std::bind(std::forward<Function>(func), std::forward<Args>(args)...));
        auto result = task.get_future();

        // Note: that the lambda takes it by reference, because it's actually stored in the bind object.
        add(std::packaged_task<void()>(std::bind([](std::packaged_task<R()>& task){task();}, std::move(task))));
        return result;
    }
#else // faster
    template<class Function, class... Args>
    void push(Function&& func, Args&&... args) {
        add(std::bind(std::forward<Function>(func), std::forward<Args>(args)...));
    }
#endif
    void wait() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (!task_queue.empty() || active > 0) {
            wait_cv.wait(lock);
        }
    }

private:
    std::vector<std::future<void>> threads;
    std::queue<std::function<void()>> task_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::condition_variable wait_cv;
    volatile bool shutdown_request {false};
    std::atomic<std::size_t> active {0};

    void add(std::function<void()>&& task) {
        // Mutual exclusion lock ownership must be acquired first
        std::lock_guard<std::mutex> lock(queue_mutex);

        // Add task into queue
        task_queue.emplace(std::forward<std::function<void()>>(task));

        // Send out signal to indicate that task has been added
        // If a thread is blocked because the task queue is empty, there will be a wake-up call.
        // If not, do nothing.
        queue_cv.notify_one();
    }

    void worker() {
        std::unique_lock<std::mutex> lock(queue_mutex);

        while (true) {
            // Process all available tasks
            active++;
            while (!task_queue.empty()) {
                // Get task from queue while mutex is still locked
                auto task = std::move(task_queue.front());
                task_queue.pop();

                // Release mutex for to let task be running without locking other threads
                queue_mutex.unlock();
                task();
                queue_mutex.lock();
            }
            active--;

            // Let waiting threads know that all tasks are complete
            if (active == 0) {
                wait_cv.notify_all();
            }

            if (shutdown_request) {
                return; // note: mutex will be released automatically
            }

            // The task queue is empty and shutdown request hasn't been received
            queue_cv.wait(lock);
        }
    }
};

}

#endif // MT_THREAD_POOL_HPP
