#include <event2/event.h>
#include <event2/bufferevent.h>
#ifndef _WIN32
#include <signal.h>
#endif // !_WIN32

#include <iostream>
using namespace std;

int main()
{

#ifdef _WIN32
	//初始化 socket 库
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#else

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		return 1;
	}
#endif
	
	//创建上下文
	event_base* base = event_base_new();
	if(base)
	{
		cout << "event_base_new success!" << endl;
	}

	void Server(event_base* base);
	Server(base);

	//循环检测事件
	if (base)
		event_base_dispatch(base);

	//释放上下文
	if (base)
		event_base_free(base);

#ifdef _WIN32
	WSACleanup();
#endif // _WIN32



	getchar();
	return 0;
}