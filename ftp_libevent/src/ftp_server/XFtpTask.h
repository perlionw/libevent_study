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
	
	//�ظ�cmd��Ϣ
	void ResCMD(std::string msg);

	//�������ͽ��������ӵ�����ͨ��
	void Send(std::string data);
	void Send(const char* data, int data_size);
	virtual void Close();

	//����Э��
	virtual void Parse(std::string type, std::string data){ }
protected:
	static void ReadCB(bufferevent* bev, void* arg);
	static void WriteCB(bufferevent* bev, void* arg);
	static void EventCB(struct bufferevent* bev, short what, void* arg);


	//��������ͨ��
	void ConnectPort();

public:
	//����bev
	struct bufferevent* bev = 0;
	std::string curDir = "/";
	std::string rootDir = ".";

	//����ͨ��
	XFtpTask* cmdTask = 0;

	//PORT ����ͨ����IP�Ͷ˿�
	std::string ip = "";
	int port = 0;

	FILE *fp = 0;

};

