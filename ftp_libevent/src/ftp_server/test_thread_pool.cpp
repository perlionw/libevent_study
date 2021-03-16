#include <event2/event.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <iostream>
#include <string.h>
#include <string>
#ifndef _WIN32
#include <signal.h>
#endif // _WIN32
#include "XThreadPool.h"
#include "XFtpServerCMD.h"
#include "XFtpFactory.h"
using namespace std;

void listener_cb(struct evconnlistener *ev, evutil_socket_t fd, struct sockaddr * s, int socklen, void *arg)
{
	std::cout << "listener_cb" << std::endl;
	XTask* task = XFtpFactory::Get()->CreateTask();
	task->sock = fd;
	XThreadPool::Get()->Dispatch(task);
}

int main()
{

#ifdef _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#else

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return 1;
#endif

	XThreadPool::Get()->Init(10);

	sockaddr_in sin;
	//需清零，否则创建服务失败
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(5001);

	event_base* base = event_base_new();

	evconnlistener* ev = evconnlistener_new_bind(base, listener_cb, 
		base, 
		BEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
		10, 
		(sockaddr*)&sin,
		sizeof(sin)
		);

	if (base)
		event_base_dispatch(base);

	if (ev)
		evconnlistener_free(ev);

	if (base)
		event_base_free(base);

#ifdef _WIN32
	WSACleanup();
#endif // _WIN32

	getchar();
	return 0;
}