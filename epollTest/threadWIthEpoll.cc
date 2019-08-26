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
#include <vector>
#include <memory>
#include <map>

using namespace std;

class Connection;
typedef shared_ptr<Connection> ConnectionPtr;

class Acceptor;
class Buffer
{
    public:
    Buffer()
    {
        printf("Buffer here\n");
    }
};
class EventLoop;

class Event
{
    public:
    //注意此处的conn类型为指针
    Event(int fd):_fd(fd)
    {
        memset(&_event, 0 , sizeof(_event));
    }
    int getFd(){return _fd;}
    //EventLoop *getLoop(){return loopptr;}
    epoll_event getEvents(){return _event;}
    int setReadCallback(function<void(ConnectionPtr,Buffer)> f);
    int setWriteCallback(function<void(ConnectionPtr,Buffer)> f)
    {
        
    }
    void setConn(ConnectionPtr conn)
    {
        _conn = conn;
    }
    void callRead(){readCallback();}
    void callWrite(){writeCallback();}
    void registerLoop(EventLoop *loopptr);
    epoll_event _event;
    //暂时为public
    function<void(void)> readCallback;
    function<void(void)> writeCallback;
    private:
    int _fd;
    
    ConnectionPtr _conn;
    //Connection *_conn;
};

class Server;

class EventLoop
{
    public:
    /*事件循环行为都一致,事件回调不同,由用户自定义*/
    //有并发问题
    EventLoop(Server *serv, string name):_serv(serv), _name(name)
    {
        epfd = epoll_create(100);
        printf("Create epfd\n");
    }
    int start()
    {
        int res = pthread_create(&pid, NULL, loop, &epfd);
        return res;
    }
    static void *loop(void *arg);

    //TODO 异常
    int addFd(epoll_event ev, int fd)
    {
        printf("Test in addFD() %d\n", fd);
        int res = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
        if (res == -1)
        {
            perror("epoll_ctl");
            exit(-1);
        }
        printf("Add Success\n");
        return res;
    }
 
    Server *getServer(){return _serv;}
    void runInLoop();
    void doTask();
    /* int addFd(int *fd)
    {
        printf("Test %d\n", *fd);
        printf("Test pthread %u\n", pthread_self());
    }*/
    private:
    string _name;
    Server *_serv;
    vector<function<void(void)> > _tasks;
    pthread_t pid;
    int epfd;
};

class Connection
{
    public:
    Connection(int fd, EventLoop *loopptr):_fd(fd), _ev(fd), _loopptr(loopptr)
    {
    }
	~Connection()
	{
		printf("=====~Connection=====\n");
	}
    void send(string str)
    {
        printf("send");
    }
    void setConn(ConnectionPtr conn)
    {
        _ev.setConn(conn);
    }
    Event *getEv(){return &_ev;}
    int getFd(){return _fd;}
    //暂时
    Buffer inputbuf;
    Buffer outputbuf;
    Event _ev;
    private:
    EventLoop *_loopptr;
    
    int _fd;
    
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

class Server
{
    public:
    Server(int num):_num(num), cur(0)
    {
        for (int i = 0; i < _num; i++)
        {
            char tmp[10] = {0};
            sprintf(tmp, "%d", i);
            string name = "thread" + string(tmp);
            //Need free()
            EventLoop *loop = new EventLoop(this, name);
            printf("Test loop %s\n", name.c_str());
            looplist[name] = loop;
        }
    }
    
    void printPtrCount()
    {
    	auto it = connlist.begin();
		printf("=====Use Count Test=====\n");
        while(it != connlist.end())
        {
            //it->first;
            printf("The use count of Conn %s is %d\n", it->first.c_str(), it->second.use_count());
            it++;         
        }
    }
    
    map<string, EventLoop *> looplist;
    map<string, ConnectionPtr> connlist;
    int start()
    {
        auto it = looplist.begin();

        while(it != looplist.end())
        {
            //it->first;
            it->second->start();
            it++;         
        }
    }
    EventLoop *getNextLoop()
    {
    	char tmp[10] = {0};
    	sprintf(tmp, "%d", cur);
    	string name = "thread" + string(tmp);
    	cur++;
    	if (cur >= _num)
    	{
    		cur = 0;
    	}
    	return looplist[name];
    }

    private:
    int _num;
    int cur;
};

class Acceptor
{
    public:
    /*回调,用于传入epoll后,事件发生时调用*/
    Acceptor(Server *serv):listenSocket(Socket(8832, "null")), _serv(serv),ev(listenSocket.getFd())
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
        int connfd = accept(listenSocket.getFd(), (struct sockaddr *) &clientaddr, &len);
        EventLoop *loop = _serv->getNextLoop();
        ConnectionPtr connptr(new Connection(connfd, loop));
        connptr->setConn(connptr);
        char tmp[10] = {0};
        sprintf(tmp, "conn%d", connfd);
        string name(tmp);
        printf("Test name : %s\n", tmp);
        
        _serv->printPtrCount();
        
        _serv->connlist[name] = connptr;
        
        _serv->printPtrCount();
        
        if (readUserCallback)
            connptr->getEv()->setReadCallback(readUserCallback);
        if (writeUserCallback)
            connptr->getEv()->setWriteCallback(writeUserCallback);

        connptr->getEv()->registerLoop(loop);
        char ip[15] = {0};
        printf("%s 连接到服务器,端口号 %d\n",inet_ntop(AF_INET, &clientaddr.sin_addr, ip ,sizeof(ip)),ntohs(clientaddr.sin_port));
    }
    void registerLoop(EventLoop *loop)
    {
        //坑:union epoll_event的data.fd和data.ptr只能取其一
        memset(&ev._event, 0, sizeof(ev._event));
        ev._event.events = EPOLLIN;
        //event.data.fd = listenSocket.getFd();
        
        printf("Test pthread %u\n", pthread_self());
        ev.readCallback = bind(&Acceptor::logic, this);
        //event.data.ptr = this;
        ev._event.data.ptr = &ev;
        loop->addFd(ev._event, listenSocket.getFd());
        printf("Test %d\n", listenSocket.getFd());
    }
    //function<void(void)> readCallback;
    int setUserReadCallback(function<void(ConnectionPtr,Buffer)> f)
    {
        readUserCallback = f;
        return 1;
    }
    int setUserWriteCallback(function<void(ConnectionPtr,Buffer)> f)
    {
        writeUserCallback = f;
        return 1;
    }
    function<void(ConnectionPtr,Buffer)> readUserCallback;
    function<void(ConnectionPtr,Buffer)> writeUserCallback;
    private:
    Event ev;
    //epoll_event event;
    //EventLoop *loopptr;
    Server *_serv;
    Socket listenSocket;
};

    int Event::setReadCallback(function<void(ConnectionPtr,Buffer)> f)
    {
    	printf("=====Use Count Test=====\n");
        printf("The use count is %d\n", _conn.use_count());
        
        readCallback = bind(f, _conn, _conn->inputbuf);
        
        printf("The use count is %d\n", _conn.use_count());
    }

    void Event::registerLoop(EventLoop *loopptr)
    {
        if (readCallback != nullptr)
        {
            _event.events |= EPOLLIN;
        }
        if (writeCallback != nullptr)
        {
            _event.events |= EPOLLOUT;
        }
        _event.data.ptr = this;
        loopptr->addFd(_event, _fd);
    }

void *EventLoop::loop(void *arg)
{
    printf("thread %u has begin\n", (int)pthread_self());
    //修改
    epoll_event events[100];
    for (; ;)
    {
        int nfds = epoll_wait(*((int*)arg), events, 188, -1);
        printf("epoll_wait return nfds %d\n", nfds);
        for (int i = 0; i < nfds; i++)
        {
            if (events[i].events & EPOLLIN)
            {
                //((Event *)events[i].data.ptr)->readCallback();
               	//这个function包含shared_ptr指针对象,此处先复制一份再调用可以延长shared_ptr的生命周期,使得调用期间不会出现core dump,调用完成之后就会析构.
                function<void(void)> func = ((Event *)events[i].data.ptr)->readCallback;
                printf("sleep 2 sec\n");
                sleep(2);
                if (func != nullptr)
                	func();
                else
                {
                	printf("End\n");
                	exit(-1);
                }
            }
            if (events[i].events & EPOLLOUT)
            {
                ((Event *)events[i].data.ptr)->writeCallback();
            }
        }
    }
    return NULL;
}

void callback(ConnectionPtr conn, Buffer buf)
{
    char tmp[10] = {0};
    read(conn->getFd(), tmp, 10);
    printf("Thread %u Recv from conn: %s\n",pthread_self(), tmp);
}

int main(void)
{
    Server serv(4);
    Acceptor acc(&serv);
    acc.setUserReadCallback(bind(callback,placeholders::_1, placeholders::_2));
    acc.registerLoop(serv.getNextLoop());
    serv.start();
    sleep(10);
    
    //此处的指针计数为3,除了connlist中的1之外，在存储回调函数的event类中还有2个对象使用到了指针.
    ConnectionPtr *tmp = &serv.connlist.begin()->second;
    //将event对象清除
    (*tmp)->_ev = Event(-1);
    //此时引用计数为1
    //(*tmp)->_ev.readCallback = bind(callback, *tmp, Buffer());
    printf("Ptr count %d\n", tmp->use_count());
    //如果loop中没有复制function，那么计数在erase之后降为0,conn析构
    //如果erase发生在复制之后，那么计数为1,在执行完函数func后，系数降为0,conn析构.
    serv.connlist.erase(serv.connlist.begin());
    printf("Ptr count %d\n", tmp->use_count());
    //serv.printPtrCount();
    while(1)
    {

    }
}
