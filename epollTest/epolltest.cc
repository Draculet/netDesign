#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
#include <string.h>

using namespace std;
const int MAX_EVENT = 30;

int main(void)
{
    char buf[100];
    memset(buf, 0, 100);
    int epfd = ::epoll_create(MAX_EVENT);
    if (epfd == -1)
    {
        ::perror("epoll_create");
        ::exit(-1);
    }
    epoll_event ev = {0}, events[MAX_EVENT] = {0};
    ev.events |= EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    ::epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    for (; ;)
    {
        int nfds = ::epoll_wait(epfd, events, MAX_EVENT, 10000);
        cout << "epoll wait return nfds: " << nfds << endl;
        if (nfds == 0)
        {
            epoll_ctl(epfd, EPOLL_CTL_DEL, STDIN_FILENO, NULL);
        }
        for (int i = 0; i < nfds; i++)
        {
            if (events[i].events & EPOLLIN)
            {
                cout << "readable" << endl;
                //read(STDIN_FILENO, buf, 100);
                //printf("Recv %s\n", buf);
                //memset(buf, 0, 100);
                ev.events |= EPOLLOUT;
                if (epoll_ctl(epfd, EPOLL_CTL_MOD, STDIN_FILENO, &ev) == -1)
                {
                    perror("epoll ctl");
                    exit(-1);
                }
            }
            if (events[i].events & EPOLLOUT)
            {
                cout << "writable" << endl;
                read(STDIN_FILENO, buf, 100);
                printf("Recv %s\n", buf);
                memset(buf, 0, 100);
                ev.events = EPOLLIN;
                if (::epoll_ctl(epfd, EPOLL_CTL_MOD, STDIN_FILENO, &ev) == -1)
                {
                    perror("epoll ctl 2");
                    exit(-1);
                }
            }
        }
    }
}