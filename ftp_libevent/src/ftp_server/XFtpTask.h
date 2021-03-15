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
	void ResCMD(std::string msg);
	//ÃüÁîbev
	struct bufferevent* cmdbev = 0;

	//½âÎöĞ­Òé
	virtual void Parse(std::string type, std::string data){ }
protected:
	static void ReadCB(bufferevent* bev, void* arg);
	static void WriteCB(bufferevent* bev, void* arg);
	static void EventCB(struct bufferevent* bev, short what, void* arg);

};

