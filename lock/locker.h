#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

#include "../noncopyable.h"

//originally check for PTHREAD_RETURN_VALUE
#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);               \
                        assert(errnum == 0); (void) errnum;})

class sem : noncopyable
{
public:
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

class locker : noncopyable
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    //internal usage
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

class locker_guard : noncopyable
{
public:
    explicit locker_guard(locker& lock) : locker_(lock)
    {
        locker_.lock();
    }

    ~locker_guard()
    {
        locker_.unlock();
    }

    //noncopyable
    locker_guard(const locker_guard&) = delete;
    locker_guard& operator=(const locker_guard&) = delete;

private:
    locker& locker_;
};

class cond : noncopyable
{
public:
    cond(const cond&) = delete;
    cond& operator=(const cond&) = delete;
    
    explicit cond(locker& mutex) : m_lock(mutex)
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    bool wait()
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_lock.get());
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool timewait(struct timespec t)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_lock.get(), &t);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    locker& m_lock;
    pthread_cond_t m_cond;
};
#endif
