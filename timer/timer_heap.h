#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <deque>
#include <queue>

#include <time.h>
#include "../log/log.h"
#include "../noncopyable.h"

class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer
{
public:
    time_t expire;
    void (* cb_func)(client_data *);
    client_data *user_data;
};

class timer_comparer
{
public:
    bool operator() (const util_timer* fst, const util_timer* sec)
    {
        return fst->expire < sec->expire;
    }
};
class timer_heap : noncopyable
{
public:
    ~timer_heap();

    void add_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void pop_timer();
    util_timer* top() const;
    bool empty() const;
    void tick();

private:
    std::priority_queue<util_timer*, std::deque<util_timer*>, timer_comparer> heap;
};

class Utils
{
public:
    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    timer_heap m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);

#endif
