#include "buffer.h"
#include "Condition.h"
#include <atomic>
#include <vector>
#include <memory>
#include <functional>
#include <pthread.h>
//TODO
#include <iostream>

namespace tmp
{
	class Thread
	{
		public:
		Thread(const std::function<void()> &func):func_(func){}
		static void *threadFunc(void *ptr)
		{
			Thread *p = static_cast<Thread *>(ptr);
			p->func_();
		}
		void start()
		{
			pthread_create(&pid_, NULL, threadFunc, this);
		}
		private:
		pthread_t pid_;
		std::function<void()> func_;
	};
}
namespace net
{
	class Logging
	{
		public:
			Logging()
			:running_(false),
			 mutex_(),
			 cond_(mutex_),
			 thread_(std::bind(&Logging::threadFunc, this)),
			 curBuf_(new LogBuffer<kSmallBuffer>()),
			 nextBuf_(new LogBuffer<kSmallBuffer>()),
			 bufVec_()
			{
			}
			
			void start()
			{
				running_ = true;
				thread_.start();
				MutexGuard mutex(mutex_);
				cond_.wait();
				//Debug
				std::cout << "Condition Return !" << std::endl; 
			}
			
			void stop()
			{
				running_ = false;
				cond_.notify();
				//线程收尾join
			}
			void append(const char *line, size_t len)
			{
				MutexGuard mutex(mutex_);
				if (len < curBuf_->avail())
				{
					curBuf_->append(line, len);
				}
				else
				{
					bufVec_.push_back(std::move(curBuf_));//移动构造,curBuf_ == nullptr
					if (nextBuf_)
					{
						curBuf_ = std::move(nextBuf_);//移动构造,nextBuf_ == nullptr
					}
					else
					{
						curBuf_.reset(new LogBuffer<kSmallBuffer>());//分配新的buffer,销毁在后台threadFunc
					}
					curBuf_->append(line, len);
					cond_.notify();
				}
			}
			
			void threadFunc()
			{
				{
					MutexGuard mutex(mutex_);
					cond_.notify();
				}
				std::unique_ptr<LogBuffer<kSmallBuffer>> buf1(new LogBuffer<kSmallBuffer>());
				std::unique_ptr<LogBuffer<kSmallBuffer>> buf2(new LogBuffer<kSmallBuffer>());
				buf1->bzero();
				buf2->bzero();
				std::vector<std::unique_ptr<LogBuffer<kSmallBuffer>>> bufToWrite;//用于交换
				while (running_)
				{
					{
						MutexGuard mutex(mutex_);
						//while (bufVec_.empty())
						//	cond_.waitForSec(5);//条件变量配合while循环防止虚假唤醒
						//debug
						printf("bufVec size : %ld\n", bufVec_.size());
						if (bufVec_.empty())
						{
							cond_.waitForSec(5);//用法特殊,因为如果第一块缓存未写满,bufVec_也是空的,但curBuf_里有日志要写
							//Debug
							std::cout << "Cond Return" << std::endl;
						}
						bufVec_.push_back(std::move(curBuf_));//此时bufVec_不可能空
						curBuf_ = std::move(buf1);
						bufVec_.swap(bufToWrite);
						if (!nextBuf_)
							nextBuf_ = std::move(buf2);
					}
					if (bufToWrite.size() > 30)
					{
						printf("Too much Log, Drop\n");
						bufToWrite.erase(bufToWrite.begin() + 2, bufToWrite.end());
						//erase会留下分配的空间,只是size()变小,仍保持弹性
					}
					//debug
					int debug = 0;
					for (const auto& buf: bufToWrite)
					{
						//TODO 临时
						//Debug
						printf("Buf length: %d\n", buf->length());
						debug++;
					}
					printf("Total Bufs num : %d\n", debug);
					
					if (bufToWrite.size() > 2)
						bufToWrite.resize(2);
					if (!buf1)
					{
						//当处理这种不确定的条件时,使用back()和pop_back()配合更好
						buf1 = std::move(bufToWrite.back());
						bufToWrite.pop_back(); 
						buf1->init();//注意unique_ptr<>->reset和unique_ptr<>.reset的区别
					}
					if (!buf2)
					{
						//当处理这种不确定的条件时,使用back()和pop_back()配合更好
						buf2 = std::move(bufToWrite.back());
						bufToWrite.pop_back(); 
						buf2->init();//注意unique_ptr<>->reset和unique_ptr<>.reset的区别
					}
					bufToWrite.clear();
					//...
				}
				//...
			}

		private:
			std::atomic<bool> running_;
			Mutex mutex_;
			Condition cond_;
			//临时实现
			tmp::Thread thread_;
			
			std::unique_ptr<LogBuffer<kSmallBuffer>> curBuf_;
			std::unique_ptr<LogBuffer<kSmallBuffer>> nextBuf_;
			std::vector<std::unique_ptr<LogBuffer<kSmallBuffer>>> bufVec_; 
	};
}

