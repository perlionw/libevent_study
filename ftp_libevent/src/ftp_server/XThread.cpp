#include "XThread.h"
#include <thread>
#include <iostream>
#include "XTask.h"
//�����߳�����Ļص�����
static void NotifyCB(evutil_socket_t fd, short which, void *arg)
{
	XThread *t = (XThread *)arg;
	t->Notify(fd, which);
}

bool XThread::Setup()
{
	//windows �����socket linux �ùܵ�
#ifdef _WIN32
	evutil_socket_t fds[2];
	if (evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0)
	{
		std::cerr << "evutil_socketpair failed !" << std::endl;
		return false;
	}

	//���óɷ�����
	evutil_make_socket_nonblocking(fds[0]);
	evutil_make_socket_nonblocking(fds[1]);
#else
	//�����ܵ� ������ send recv��ȡ ��Ҫ��read write

	int fds[2];

	if (pipe(fds))
	{
		std::cerr << "pipe failed! " << std::endl;
		return false;
	}
#endif

	//��ȡ�󶨵�event�¼��У�д��Ҫ����
	notify_send_fd = fds[1];

	//����libevent������(����)
	event_config* ev_conf = event_config_new();
	event_config_set_flag(ev_conf, EVENT_BASE_FLAG_NOLOCK);
	this->base = event_base_new_with_config(ev_conf);
	if(!base)
	{
		std::cerr << "event_base_new_with_config failed in thread!" << std::endl;
		return false;
	}

	//��ӹܵ������¼������ڼ����߳�ִ������
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

	//�����߳�
	std::thread th(&XThread::Main, this);

	//�̷߳���
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
	//ˮƽ���� ֻҪû�н�����ɣ����ٴν���
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
	//��ȡ���񣬲���ʼ������
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
