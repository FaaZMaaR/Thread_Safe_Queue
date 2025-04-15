#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

class SafeQueue {
private:
    std::queue<std::function<void(int)>> taskQueue{};
    std::mutex mutex{};
    std::condition_variable condVar{};

public:
    void push(std::function<void(int)> task) {
        std::unique_lock<std::mutex> ulock{ mutex };
        taskQueue.push(task);
        ulock.unlock();
        condVar.notify_all();
    }
    std::function<void(int)> pop() {
        std::unique_lock<std::mutex> ulock{ mutex };
        condVar.wait(ulock, [&] {return !taskQueue.empty(); });        
        std::function<void(int)> task = taskQueue.front();
        taskQueue.pop();
        ulock.unlock();
        condVar.notify_all();
        return task;
    }
};

class ThreadPool {
private:
    std::vector<std::thread> threads{};
    SafeQueue safeQueue{};

public:
    ThreadPool() {
        int threadsCount = std::thread::hardware_concurrency();
        for (int i = 0; i < threadsCount; ++i) {
            threads.push_back(std::thread(&ThreadPool::work, this, i + 1));
        }
    }
    ~ThreadPool() {
        for (auto& thread : threads) {
            thread.detach();
        }
    }
    void work(int number) {
        while (true) {
            safeQueue.pop()(number);
        }
    }
    void submit(std::function<void(int)> task) {
        safeQueue.push(task);
    }
};

void testFunc1(int number) {
    std::cout << __func__ << " was called by thread " << number << std::endl;
}

void testFunc2(int number) {
    std::cout << __func__ << " was called by thread " << number << std::endl;
}

int main() {
    ThreadPool threadPool{};
    for (int i = 0; i < 10; ++i) {
        threadPool.submit(testFunc1);
        threadPool.submit(testFunc2);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}