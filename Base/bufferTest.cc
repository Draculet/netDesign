#include "buffer.h"
#include <iostream>
using namespace net;
using namespace std;
int main(void)
{
	LogBuffer<40> buf;
	for (int i = 0; i < 10; i++)
	{
	buf.append("hello,world\n", 12);
	buf.append("hello,world\n", 12);
	buf.append("hello,world\n", 12);
	buf.append("he\n", 3);
	buf.append("132133812839123udchuasudhiasjd82hdnjabd812\n", 43);
	buf.append("dsancjasjcbasuidh9qhe9dqihd81uh1ud8h1289hd9ajdiasjdoasdj8h2udhas\n", 65);
	buf.append("hello,world\n", 12);
	buf.append("132133812839123udchuasudhiasjd82hdnjabd812\n", 43);
	buf.append("dsancjasjcbasuidh9qhe9dqihd81uh1ud8h1289hd9ajdiasjdoasdj8h2udhas\n", 65);
	}
	cout << buf.data();
}
