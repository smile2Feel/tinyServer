#include "timer_heap.h"
#include "../http/http_conn.h"
#include <string.h>

timer_heap::~timer_heap()
{
    util_timer* iter;
    while (!empty())
    {
        iter = heap.top();
        delete iter;
        heap.pop();
    }
}

void timer_heap::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    heap.push(timer);
}

void timer_heap::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    // delay-delete, heap only support delete the toppest element.
    timer->cb_func = NULL;
}

void timer_heap::pop_timer(void)
{
    if (!empty())
    {
        heap.pop();
    }
}

util_timer* timer_heap::top() const
{
    return heap.top();
}

bool timer_heap::empty() const
{
    return heap.empty();
}

void timer_heap::tick()
{
    if (empty())
    {
        return;
    }
    
    time_t cur = time(NULL);
    util_timer *tmp;
    while (!empty())
    {
        tmp = top();
        if (cur < tmp->expire)
        {
            break;
        }
        if (tmp->cb_func)
        {
            tmp->cb_func(tmp->user_data);
        }
        pop_timer();
        delete tmp;
    }
}

void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    time_t cur = time(NULL);
    m_timer_lst.tick();
    // adapt the timeSlot to nearest timer
    time_t interval = m_timer_lst.top()->expire - cur; 
    alarm(interval);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}