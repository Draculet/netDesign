#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace net
{
	const int kSmallBuffer = 4000;
	const int kLargeBuffer = 4000 * 1000;
	
	template<size_t SIZE> 
	class LogBuffer
	{
		public:
			LogBuffer():cur_(data_){bzero();}
			LogBuffer(const LogBuffer &)=delete;
			LogBuffer(LogBuffer &&)=delete;
			~LogBuffer()
			{
				//Debug
				printf("~LogBuffer()\n");
			}
			void init()
			{
				bzero();
				cur_ = data_;
			}
			void operator=(const LogBuffer &)=delete;
			char *current(){return cur_;}
			size_t avail(){return end() - cur_;}
			void bzero(){memset(data_, 0, sizeof(data_));}
			int length(){return cur_ - data_;}
			void append(const char * buf, size_t len)
			{
				if (avail() > len)
				{
					memcpy(cur_, buf, len);
					cur_ += len;
				}
			}
			char *data(){return data_;}
			
		private:
			const char *end(){return data_ + sizeof(data_);}
			char data_[SIZE];
			char *cur_;
	};
}
