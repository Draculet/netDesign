#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
#include <string.h>
#include <sys/timerfd.h>
#include <sys/time.h>

#include <stdint.h>

using namespace std;
const int MAX_EVENT = 30;


/*
struct timespec {
               time_t tv_sec;                 //Seconds 
               long   tv_nsec;                //Nanoseconds 
           };
           struct itimerspec {
               struct timespec it_interval;   //Interval for periodic timer 
               struct timespec it_value;      //Initial expiration 
           };
*/
int main(void)
{

	struct itimerspec new_value;
	struct itimerspec new_value2;
   	struct timespec now;
   	struct timespec now2;
   	//uint64_t exp, tot_exp;
   	//ssize_t s;

	
	clock_gettime(CLOCK_MONOTONIC, &now2);
	cout << "clock_gettime() s: " << now2.tv_sec << endl;
	cout << "clock_gettime() ns: " << now2.tv_sec << endl;
	
	
	clock_gettime(CLOCK_REALTIME, &now);
	cout << "clock_gettime() s: " << now.tv_sec << endl;
	cout << "clock_gettime() ns: " << now.tv_sec << endl;
	
	
   	new_value.it_value.tv_sec = now.tv_sec + 5;//5s init
   	new_value.it_value.tv_nsec = now.tv_nsec;
   	
   	new_value2.it_value.tv_sec = now2.tv_sec + 5;//5s init
   	new_value2.it_value.tv_nsec = now2.tv_nsec;
   	
    new_value.it_interval.tv_sec = 5;//5s loop
    new_value.it_interval.tv_nsec = 0;
    
    new_value2.it_interval.tv_sec = 0;//0s loop
    new_value2.it_interval.tv_nsec = 0;

   	int fd = timerfd_create(CLOCK_REALTIME, 0);
	int fd2 = timerfd_create(CLOCK_MONOTONIC, 0);
	
   	timerfd_settime(fd, 1, &new_value, NULL);
   	
   	timerfd_settime(fd2, 1, &new_value2, NULL);

   	printf("timer started\n");
   	
   	printf("timer2 started\n");

   /*for (tot_exp = 0; tot_exp < max_exp;) {
       s = read(fd, &exp, sizeof(uint64_t));
       if (s != sizeof(uint64_t))
           handle_error("read");

       tot_exp += exp;
       print_elapsed_time();
       printf("read: %llu; total=%llu\n",
               (unsigned long long) exp,
               (unsigned long long) tot_exp);
   }
   */
   
   
    char buf[100];
    memset(buf, 0, 100);
    int epfd = epoll_create(MAX_EVENT);
    if (epfd == -1)
    {
        perror("epoll_create");
        exit(-1);
    }
    epoll_event ev = {0}, ev2 = {0}, events[MAX_EVENT] = {0};
    ev.events |= EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    
    ev2.events |= EPOLLIN;
    ev2.data.fd = fd2;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd2, &ev2);
    
    //sleep(10);
    for (; ;)
    {
        int nfds = epoll_wait(epfd, events, MAX_EVENT, 10000);
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
                int res = read(events[i].data.fd , buf, 100);
                //cout << "read " << res <<" bytes" << endl;
                uint64_t n = 0;
                memcpy(&n, buf, 8);
                cout << "OverTime Times :" << n << endl;
                struct timeval tv;
				gettimeofday(&tv, NULL);
				long long ts = (long long)tv.tv_sec*1000 + tv.tv_usec/1000;
				printf("Current time: %lld ms\n", ts);
                //printf("Recv %s\n", buf);
                memset(buf, 0, 100);
                //ev.events |= EPOLLOUT;
                //if (epoll_ctl(epfd, EPOLL_CTL_MOD, STDIN_FILENO, &ev) == -1)
                //{
                //    perror("epoll ctl");
                //    exit(-1);
                //}
            }
            if (events[i].events & EPOLLOUT)
            {
                cout << "writable" << endl;
                //read(STDIN_FILENO, buf, 100);
                //printf("Recv %s\n", buf);
                //memset(buf, 0, 100);
                //ev.events = EPOLLIN;
                //if (::epoll_ctl(epfd, EPOLL_CTL_MOD, STDIN_FILENO, &ev) == -1)
                //{
                //    perror("epoll ctl 2");
                //    exit(-1);
                //}
            }
        }
    }
}
