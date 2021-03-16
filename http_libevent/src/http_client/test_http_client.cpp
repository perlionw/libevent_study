#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <string>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif
#include <iostream>
using namespace std;

int main()
{
#ifdef _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return 1;
#endif

	//创建上下文
	event_base* base = event_base_new();
	if (!base)
	{
		std::cout << "event_base_new failed!" << std::endl;
		return -1;
	}

	//生成请求消息 GET

	

	getchar();
	return 0;
}