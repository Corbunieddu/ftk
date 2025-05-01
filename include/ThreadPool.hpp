#ifndef FTK_THREADPOOL_HPP
#define FTK_THREADPOOL_HPP

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <string>
#include <atomic>
#include <shared_mutex>
#include <fstream>


class ThreadPool {
    std::vector<std::thread> workers;        // Thread fissi
    std::queue<std::function<void()>> tasks; // Coda dei compiti
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop = false; // Flag per terminare il pool

public:
    // Costruttore: inizializza i thread
    explicit ThreadPool(size_t threads);
    size_t get_nthread();

    template <typename F>
    inline void enqueue(F&& task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace(std::forward<F>(task));
        }
        condition.notify_one(); // Notifica un thread
    }

    // Distruttore: termina tutti i thread
    ~ThreadPool();
};

#endif // FTK_THREADPOOL_HPP