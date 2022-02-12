#include "ThreadPooll.h"

void Worker::operator()()
{
    ThreadPool::Task task;
    while (true)
    {
        {   // acquire the queue lock
            std::unique_lock<std::mutex> lock(pool.queue_mutex);
            // wait for the notification
            while (!pool.stop && pool.tasks.empty())
            {
                pool.cond.wait(lock);
            }

            if (pool.stop)
                return;

            // pop a task and execute
            task = pool.tasks.front();
            pool.tasks.pop_front();
        }   // release the queue lock

        task();
    }
}

// create numbers of work threads.
ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    for (size_t i = 0; i < threads; i++)
        workers.push_back(std::thread(Worker(*this)));
}

// stop and join all the threads
ThreadPool::~ThreadPool()
{
    stop = true;
    cond.notify_all();

    // join all the threads.
    for (auto &worker : workers)
        worker.join(); 
}

template<typename F>
void ThreadPool::enqueue(F f)
{
    {   // acquire lock
        std::unique_lock<std::mutex> lock(queue_mutex);
        workers.push_back(std::function<void()>(f));
    }   // release lock

    // wake up one thread
    cond.notify_one();
}