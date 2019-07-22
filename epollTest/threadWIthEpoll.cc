#include <pthread.h>
#include <sys/epoll.h>
#include <functional>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

class Acceptor;
class Event
{
    public:
    Event()
    {

    }
    int getFd(){return fd;}
    epoll_event getEvents{return events;}
    int setReadCallback(function<>);
    int setWriteCallback(function<>);
    private:
    int fd;
    epoll_event event;
    function<void(void)> readCallback;
    function<void(void)> writeCallback;
    EventLoop *loopptr;
};
class EventLoop
{
    public:
    /*事件循环行为都一致,事件回调不同,由用户自定义*/
    EventLoop()
    {
        epfd = epoll_create(100);
        int res = pthread_create(&pid, NULL, loop, &epfd);
    }
    static void *loop(void *arg);

    //TODO 异常
    int addFd(epoll_event ev, int fd)
    {
        printf("Test %d\n", fd);
        int res = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
        if (res == -1)
        {
            perror("epoll_ctl");
            exit(-1);
        }
        printf("Add Success\n");
        return res;
    }
    /* int addFd(int *fd)
    {
        printf("Test %d\n", *fd);
        printf("Test pthread %u\n", pthread_self());
    }*/
    private:
    pthread_t pid;
    int epfd;
};
class Socket
{
    public:
    Socket(int port, string ip)
    {
        struct sockaddr_in serveraddr;
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
            perror("socket");
            exit(-1);
        }
        bzero(&serveraddr,sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port);
        //WARN
        //serveraddr.sin_addr.s_addr = htonl(ip.c_str());
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        int ret=bind(fd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));//绑定IP和端口
        if(ret!=0)
        {
            close(fd);
            printf("bind error:%s\n",strerror(errno));
            exit(-1);
        }
    }
    int getFd(){return fd;}
    private:
    int fd;
};

class Acceptor
{
    public:
    /*回调,用于传入epoll后,事件发生时调用*/
    Acceptor():listenSocket(Socket(8832, "null"))
    {
        int ret = listen(listenSocket.getFd(), 20);
        if(ret!=0)
        {
            close(listenSocket.getFd());
            printf("listen error:%s\n",strerror(errno));
            exit(-1);
        }
        printf("begin listening\n");
    }
    void logic()
    {
        struct sockaddr_in clientaddr;
        socklen_t len = sizeof(clientaddr);
        bzero(&clientaddr,sizeof(clientaddr));
        accept(listenSocket.getFd(), (struct sockaddr *) &clientaddr, &len);
        char ip[15] = {0};
        printf("%s 连接到服务器,端口号 %d\n",inet_ntop(AF_INET, &clientaddr.sin_addr, ip ,sizeof(ip)),ntohs(clientaddr.sin_port));
    }
    void registerLoop(EventLoop *loop)
    {
        //坑:union epoll_event的data.fd和data.ptr只能取其一
        memset(&event, 0, sizeof(event));
        event.events = EPOLLIN;
        //event.data.fd = listenSocket.getFd();
        
        printf("Test pthread %u\n", pthread_self());
        readCallback = bind(&Acceptor::logic, this);
        event.data.ptr = this;
        loop->addFd(event, listenSocket.getFd());
        printf("Test %d\n", listenSocket.getFd());
        //loop->addFd(event.data.fd);
    }
    function<void(void)> readCallback;
    private:
    epoll_event event;
    EventLoop *loopptr;
    Socket listenSocket;
};

    void *EventLoop::loop(void *arg)
    {
        printf("thread %u has begin\n", (int)pthread_self());
        epoll_event events[100];
        for (; ;)
        {
            int nfds = epoll_wait(*((int*)arg), events, 100, -1);
            for (int i = 0; i < nfds; i++)
            {
                if (events[i].events & EPOLLIN)
                {
                    ((Acceptor *)events[i].data.ptr)->readCallback();
                }
                if (events[i].events & EPOLLOUT)
                {
                    events[i].data.ptr->writeCallback();
                }
            }
        }
        return NULL;
    }

int main(void)
{
    EventLoop loop;
    sleep(1);
    Acceptor acc;
    acc.registerLoop(&loop);
    for (int i = 0;i < 4; i++)
    {
        EventLoop loop;
    }
    while(1)
    {

    }
}