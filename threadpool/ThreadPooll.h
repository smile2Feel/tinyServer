#ifndef Thread_Pooll_H
#define Thread_Pooll_H

#include <cstddef>
#include <vector>
#include <deque>
#include <functional>

#include <mutex>
#include <condition_variable>
#include <thread>

#include "../noncopyable.h"

class ThreadPool;
class Worker : noncopyable
{
public:
    explicit Worker(ThreadPool &s) : pool(s) { }
    void operator()();
private:
    ThreadPool &pool;
};

class ThreadPool : noncopyable
{
public:
    typedef std::function<void()> Task;
    friend class Worker;
    ThreadPool(size_t);
    template <typename F>
    void enqueue(F f);
    ~ThreadPool();
private:
    // keep track of threads so we can join them
    std::vector<std::thread> workers;

    // task queue
    //todo: why deque?
    std::deque<Task> tasks;

    // synchronization
   std::mutex queue_mutex;
   std::condition_variable cond;
   bool stop;
};

#endif
