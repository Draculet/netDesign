#include "Log.h"

using namespace net;
using namespace std;
using namespace tmp;

void func(Logging *log)
{
	while(1)
	{
		log->append("abcdefghijklmnopkrstuvwxyzabcdefghijklmnopkrstuvwxyzabcdefghijklmnopkrstuvwxyz\n", 27*3);
	}
}
int main(void)
{
	Logging log;
	log.start();
	
	Thread t(bind(func, &log));
	t.start();

	while(1)
	{
		log.append("abcdefghijklmnopkrstuvwxyzabcdefghijklmnopkrstuvwxyzabcdefghijklmnopkrstuvwxyz\n", 27*3);
	}
	return 0;
}

