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
//TODO 代码整体需修改

using namespace std;

class Connection;
typedef shared_ptr<Connection> ConnectionPtr;

class Acceptor;
//TODO 补充Buffer
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
    Event(int fd):_fd(fd)
    {
        memset(&_event, 0 , sizeof(_event));
    }
    int getFd(){return _fd;}
    //EventLoop *getLoop(){return loopptr;}
    epoll_event getEvents(){return _event;}
    //TODO 传递回调方式欠佳
    int setReadCallback(function<void(ConnectionPtr,Buffer)> f);
    int setWriteCallback(function<void(ConnectionPtr,Buffer)> f)
    {
        
    }
    //TODO 这种方式传智能指针欠缺优雅
    void setConn(ConnectionPtr conn)
    {
        _conn = conn;
    }
    void callRead(){readCallback();}
    void callWrite(){writeCallback();}
    void registerLoop(EventLoop *loopptr);
    //TODO 待修改,为了节约时间先写为public
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
    EventLoop(Server *serv, string name):_serv(serv), _name(name)
    {
        epfd = epoll_create(100);
        printf("Create epfd\n");
    }
    //待封装线程库
    int start()
    {
        int res = pthread_create(&pid, NULL, loop, &epfd);
        return res;
    }
    static void *loop(void *arg);

    //TODO 此处注意epoll_event 的data.ptr data.fd只能填写一个(union)
    //TODO 传指针还是传引用
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
    //TODO Eventloop的任务队列部分
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

    void send(string str)
    {
        printf("send");
    }
    //TODO 设置回调不优雅
    void setConn(ConnectionPtr conn)
    {
        _ev.setConn(conn);
    }
    Event *getEv(){return &_ev;}
    int getFd(){return _fd;}
    //TODO 暂时为public
    Buffer inputbuf;
    Buffer outputbuf;
    private:
    EventLoop *_loopptr;
    Event _ev;
    int _fd;
    
};


class Socket
{
    public:
    //TODO 这里实现偷懒了
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
    //TODO 可优化调度?
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
    //TODO Acceptor的编写较为粗糙
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
        _serv->connlist[name] = connptr;
        
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
    
    //TODO 传回调方式待优化
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
        readCallback = bind(f, _conn, _conn->inputbuf);
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
                ((Event *)events[i].data.ptr)->readCallback();
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
    while(1)
    {

    }
}
