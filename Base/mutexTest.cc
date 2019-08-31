#include "Mutex.h"
#include <functional>
#include <pthread.h>

using namespace std;
using namespace common;
using namespace net;

void threadFunc(Mutex *m)
{
	printf("thread %d started\n", gettid());
	MutexGuard mutex(*m);
	printf("thread %d own the lock\n", m->getOwner());
	while(1)
	{
		printf("thread %d own the lock\n", m->getOwner());
		sleep(10);
	}
}

struct tmp
{
	tmp(function<void()> f):f_(f){}
	static void *threadFunc(void *p)
	{
		tmp *tp = static_cast<tmp *>(p);
		tp->f_();
	}
	
	function<void()> f_;
};

int main(void)
{
	Mutex m;
	{
		MutexGuard mutex(m);
		printf("thread pid: %d\n", m.getOwner());
	}
	pthread_t pt;
	function<void()> f = bind(threadFunc, &m);//使用bind会复制mutex,所以需要传指针
	tmp t(f);
	for (int i = 0; i < 10; i++)
		pthread_create(&pt, NULL, &tmp::threadFunc, &t);
	while(1)
	{}
}

