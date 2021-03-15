
#include "XFtpServerCMD.h"
#include <event2/bufferevent.h>
#include <event2./event.h>
#include<iostream>
using namespace std;

void ReadCB(struct bufferevent *bev, void *ctx)
{
	XFtpServerCMD* cmd = (XFtpServerCMD*)ctx;
	char data[1024] = { 0 };

	for (;;)
	{
		int len = bufferevent_read(bev, data, sizeof(data) - 1);
		if (len <= 0) break;
		data[len] = '\0';
		cout << data << endl;

		if (strstr(data, "q"))
		{
			bufferevent_free(bev);
			delete cmd;
			break;
		}
	}
}

//初始化任务
bool XFtpServerCMD::Init()
{
	cout << "XFtpServerCMD::Init()" << endl;
	//监听socket bufferevent
	// base socket
	bufferevent* bev = bufferevent_socket_new(base, sock, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, ReadCB, 0, 0, this);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	return true;
}

XFtpServerCMD::XFtpServerCMD()
{
}


XFtpServerCMD::~XFtpServerCMD()
{
}
