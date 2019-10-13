#include <arpa/inet.h>
#include <stdio.h>
#include <iostream>
using namespace std;
/*
sockaddr_in(在netinet/in.h中定义)
struct sockaddr_in {
short int sin_family;                      // Address family
unsigned short int sin_port;       // Port number 
struct in_addr sin_addr;              // Internet address 
unsigned char sin_zero[8];         // Same size as struct sockaddr
};

struct in_addr {
unsigned long s_addr;
};
*/

//结论: 大端小端问题核心在于不同机器对于多个字节组成的元素(如:int long等)的解读方式不同,有的是从高字节开始往低字节解读(小端)，有的从低字节开始解读(大端)
//我们的x86架构机子是小端存储,网络字节序是小端.本机在发送这类数据之前,先统一转成网络字节序,客户端接受到后再进行转换即可保证数据不乱.



int main(void)
{
    //printf("%d\n", sizeof(unsigned long));//8 bytes
    //printf("%d\n", INADDR_ANY);//值为0
    //二进制于十六进制是以 4位 为单位转换的
    int n2 = 1;
    string ip = "1.1.4.4";
    auto n = inet_addr(ip.c_str());
    printf("%d\n", n);
    printf("%08x\n", n);//对比下面的输出可以看出大小端的区别
    //输出: 04040101  //注意:二进制于十六进制是以 4位 为单位转换的
    //printf("%d\n", sizeof(n));//in_addr_t 4 bytes
    char *p = (char *)&n;
    for (int i = 0; i < sizeof(n); i++)
    {
        printf("%d.", *(p+i));
    }
    //输出: 1.1.4.4. 即存储方式:
    //内存从低到高分别是0x 01010404
    printf("\n");
    //简单的字节序转换
    char *p1 = (char *)&n;
    char *p2 = p1 + 3;
    while (p1 < p2)
    {
        char t;
        t = *p1;
        *p1 = *p2;
        *p2 = t;
        p1++;
        p2--;
    }
    printf("%08x\n", n);
    //printf("%x\n", n);

    //进一步实验:
    int n3 = 0x01010404;//本应对应1.1.4.4
    char *p3 = (char *)&n3;
    for (int i = 0; i < sizeof(n); i++)
    {
        printf("%d.", *(p3+i));//输出4.4.1.1
    }

    //TODO pton ntop等函数的实现
}