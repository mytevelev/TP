#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <atomic>
#include <chrono>
using namespace std;

// Потокобезопасная очередь
template<typename T>
class safe_queue {
private:
    queue<T> q;
    mutex mymutex;
    condition_variable cond_var;

public:
    safe_queue() = default;

    void push(const T& value) {
        {
            lock_guard<mutex> lock(mymutex);
            q.push(value);
        }
        cond_var.notify_one();
    }

    bool pop(T& result) {
        unique_lock<mutex> lock(mymutex);
        cond_var.wait(lock, [this]() { return !q.empty(); });
        if (!q.empty()) {
            result = move(q.front());
            q.pop();
            return true;
        }
        return false;
    }

    bool empty() {
        lock_guard<mutex> lock(mymutex);
        return q.empty();
    }
};

// Пул потоков
class thread_pool {
private:
    vector<thread> workers;
    safe_queue<function<void()>> task_queue;
    atomic<bool> stop;

    void work() 
    {
        while (true) 
        {
            function<void()> task;
            if (task_queue.pop(task)) 
            {
                if (!task) 
                {
                    // nullptr task indicates shutdown signal
                    break;
                }
                try 
                {
                    task();
                }
                catch (const exception& e) {
                    cerr << "Exception in task: " << e.what() << endl;
                }
            }
        }
    }

public:
    thread_pool(size_t num_threads = thread::hardware_concurrency()) : stop(false) 
    {
        for (size_t i = 0; i < num_threads; ++i) 
        {
            workers.emplace_back(&thread_pool::work, this);
        }
    }

    ~thread_pool() 
    {
        stop = true;
        for (size_t i = 0; i < workers.size(); ++i) 
        {
            // Push nullptr tasks to signal threads to exit
            task_queue.push(nullptr);
        }
        for (auto& worker : workers) 
        {
            if (worker.joinable()) 
            {
                worker.join();
            }
        }
    }

    void submit(const function<void()>& task) 
    {
        task_queue.push(task);
    }
};

// Тестовые функции
void test_function_1() 
{
    cout << "Function 1 is executing on thread " << this_thread::get_id() << endl;
}

void test_function_2() {
    cout << "Function 2 is executing on thread " << this_thread::get_id() << endl;
}

int main() {
    // Determine the number of cores
    unsigned int num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0) 
    {
        std::cout << "Unable to determine the number of cores. Defaulting to 1 thread." << std::endl;
        num_cores = 1; // Fallback to 1 thread
    }
    else 
    {
        std::cout << "Number of processor cores: " << num_cores << std::endl;
    }

    // Create a thread pool with the number of threads equal to the number of cores
    thread_pool p(num_cores);

    // Submit tasks to the thread pool
    for (int i = 0; i < 10; ++i) {
        p.submit(test_function_1);
        p.submit(test_function_2);
        this_thread::sleep_for(chrono::seconds(1));
    }

    // Thread pool will shut down gracefully when it goes out of scope
    return 0;
}
