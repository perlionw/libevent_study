#include "XThread.h"
#include <thread>
#include <iostream>
#include "XTask.h"
//激活线程任务的回调函数
static void NotifyCB(evutil_socket_t fd, short which, void *arg)
{
	XThread *t = (XThread *)arg;
	t->Notify(fd, which);
}

bool XThread::Setup()
{
	//windows 用配对socket linux 用管道
#ifdef _WIN32
	evutil_socket_t fds[2];
	if (evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0)
	{
		std::cerr << "evutil_socketpair failed !" << std::endl;
		return false;
	}

	//设置成非阻塞
	evutil_make_socket_nonblocking(fds[0]);
	evutil_make_socket_nonblocking(fds[1]);
#else
	//创建管道 不能用 send recv读取 需要用read write

	int fds[2];

	if (pipe(fds))
	{
		std::cerr << "pipe failed! " << std::endl;
		return false;
	}
#endif

	//读取绑定到event事件中，写入要保存
	notify_send_fd = fds[1];

	//创建libevent上下文(无锁)
	event_config* ev_conf = event_config_new();
	event_config_set_flag(ev_conf, EVENT_BASE_FLAG_NOLOCK);
	this->base = event_base_new_with_config(ev_conf);
	if(!base)
	{
		std::cerr << "event_base_new_with_config failed in thread!" << std::endl;
		return false;
	}

	//添加管道监听事件，用于激活线程执行任务
	event* ev = event_new(base, fds[0], EV_READ | EV_PERSIST, NotifyCB, this);
	event_add(ev, 0);

	return true;
}

void XThread::Main()
{
	std::cout << id << "XThread::Main begin" << std::endl;
	event_base_dispatch(base);
	event_base_free(base);
	std::cout << id << "XThread::Main end" << std::endl;
}

void XThread::Start()
{
	Setup();

	//启动线程
	std::thread th(&XThread::Main, this);

	//线程分离
	th.detach();

}

void XThread::Activate()
{

#ifdef _WIN32
	int re = send(this->notify_send_fd, "c", 1, 0);
#else
	int re = write(this->notify_send_fd, "c", 1);
#endif

	if (re <= 0)
		std::cerr << "XThread::Activate() failed!" << std::endl;

}

void XThread::Notify(evutil_socket_t fd, short which)
{
	//水平触发 只要没有接受完成，会再次进来
	char buf[2] = { 0 };

#ifdef _WIN32
	int re = recv(fd, buf, 1, 0);
#else
	int re = read(fd, nif, 1);
#endif

	if (re <= 0)
		return;

	std::cout << id << " thread " << buf << std::endl;
	XTask* task = NULL;
	//获取任务，并初始化任务
	tasks_metux.lock();
	if (tasks.empty())
	{
		tasks_metux.unlock();
		return;
	}
	task = tasks.front();
	tasks.pop_front();
	tasks_metux.unlock();
	task->Init();
}

void XThread::AddTask(XTask* t)
{
	if (!t) return;
	t->base = this->base;
	t->thread_id = this->id;
	tasks_metux.lock();
	tasks.push_back(t);
	tasks_metux.unlock();	
}
