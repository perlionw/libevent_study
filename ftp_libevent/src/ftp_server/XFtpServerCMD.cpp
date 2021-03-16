
#include "XFtpServerCMD.h"
#include <event2/bufferevent.h>
#include <event2/event.h>
#include<iostream>
#include <string>
using namespace std;

//初始化任务
bool XFtpServerCMD::Init()
{
	cout << "XFtpServerCMD::Init()" << endl;
	//监听socket bufferevent
	// base socket
	bufferevent* bev = bufferevent_socket_new(base, sock, BEV_OPT_CLOSE_ON_FREE);
	this->bev = bev;
	this->SetCallback(bev);

	//添加超时
	timeval rt = { 60, 0 };
	bufferevent_set_timeouts(bev, &rt, 0);
	string msg = "220 Welcome to libevent XFtpServer\r\n";
	bufferevent_write(bev, msg.c_str(), msg.size());
	return true;
}

void XFtpServerCMD::Read(struct bufferevent* bev)
{
	char data[1024] = { 0 };

	for (;;)
	{
		int len = bufferevent_read(bev, data, sizeof(data) - 1);
		if (len <= 0) break;
		data[len] = '\0';
		cout << data << endl;

		//分发到处理对象， 分析出类型 USER anonymous
		std::string type = "";
		for (int i = 0; i < len; ++i)
		{
			if (data[i] == ' ' || data[i] == '\r')
				break;
			type += data[i];
		}
	
		cout << "type is [" << type << "]" << endl;
		if (calls.find(type) != calls.end())
		{
			XFtpTask* t = calls[type];
			t->cmdTask = this; //用来处理回复命令和目录
			t->ip = ip;
			t->port = port;
			t->base = base;
			t->Parse(type, data);
			
			if (type == "PORT")
			{
				ip = t->ip;
				port = t->port;
			}
		}
		else
		{
			string msg = "200 OK\r\n";
			bufferevent_write(bev, msg.c_str(), msg.size());
		}
	}
}

void XFtpServerCMD::Write(struct bufferevent* bev)
{

}

void XFtpServerCMD::Event(struct bufferevent* bev, short what)
{
	//如果对方网络断掉，或则机器死机有可能收不到 BEV_EVENT_EOF 数据
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << "BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT" << endl;
		delete this;
	}
}

void XFtpServerCMD::Reg(std::string cmd, XFtpTask* call)
{
	if (!call)
	{
		std::cout << "XFtpServerCMD::Reg call is null " << std::endl;
		return;
	}

	if (cmd.empty())
	{
		std::cout << "XFtpServerCMD::Reg cmd is null" << std::endl;
		return;
	}

	//已经注册的是否覆盖 不覆盖，提示错误
	if (calls.find(cmd) != calls.end())
	{
		std::cout << cmd << " is alreay register" << std::endl;
		return;
	}

	calls[cmd] = call;
	calls_del[call] = 0;
}

XFtpServerCMD::XFtpServerCMD()
{
}


XFtpServerCMD::~XFtpServerCMD()
{
	Close();
	for (auto call : calls_del)
	{
		call.first->Close();
		delete call.first;
	}

}
