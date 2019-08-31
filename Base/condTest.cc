#include "Condition.h"
#include <functional>

using namespace std;
using namespace net;
using namespace common;

void threadFunc(Mutex *m, Condition *cond)
{
	printf("thread %d started\n", gettid());
	{
		MutexGuard mutex(*m);
		printf("thread %d own the lock\n", m->getOwner());
		//cond->wait();
		cond->waitForSec(5);
		printf("thread %d return\n", gettid());
		printf("And thread %d own the lock\n", m->getOwner());
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
	Condition c(m);
	pthread_t pt;
	function<void()> f = bind(threadFunc, &m, &c);
	tmp t(f);
	for (int i = 0; i < 10; i++)
		pthread_create(&pt, NULL, &tmp::threadFunc, &t);
	while(1)
	{
		printf("main: thread %d own the lock\n", m.getOwner());
		sleep(1);
	}
}
