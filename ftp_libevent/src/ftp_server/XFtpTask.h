#pragma once
#include "XTask.h"
#include <iostream>
class XFtpTask :
	public XTask
{

public:

	virtual bool Init() { return true;  }
	virtual void Read(struct bufferevent* bev) {}
	virtual void Write(struct bufferevent* bev) {}
	virtual void Event(struct bufferevent*bev, short what) {}
	void SetCallback(struct bufferevent* bev);
	
	//回复cmd消息
	void ResCMD(std::string msg);

	//用来发送建立了连接的数据通道
	void Send(std::string data);
	void Send(const char* data, int data_size);
	virtual void Close();

	//解析协议
	virtual void Parse(std::string type, std::string data){ }
protected:
	static void ReadCB(bufferevent* bev, void* arg);
	static void WriteCB(bufferevent* bev, void* arg);
	static void EventCB(struct bufferevent* bev, short what, void* arg);


	//连接数据通道
	void ConnectPort();

public:
	//命令bev
	struct bufferevent* bev = 0;
	std::string curDir = "/";
	std::string rootDir = ".";

	//命令通道
	XFtpTask* cmdTask = 0;

	//PORT 数据通道的IP和端口
	std::string ip = "";
	int port = 0;

	FILE *fp = 0;

};

