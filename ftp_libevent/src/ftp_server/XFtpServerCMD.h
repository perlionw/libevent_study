
#pragma once
#include "XFtpTask.h"
#include <map>
class XFtpServerCMD :public XFtpTask
{
public:
	//��ʼ������
	virtual bool Init();
	virtual void Read(struct bufferevent* bev);
	virtual void Write(struct bufferevent* bev);
	virtual void Event(struct bufferevent* bev, short what);

	//ע���������� ����Ҫ�����̰߳�ȫ�����ǻ�δ�ַ����߳�
	void Reg(std::string, XFtpTask* call);
	XFtpServerCMD();
	~XFtpServerCMD();

private:
	std::map<std::string, XFtpTask*> calls;

	//������Դ����
	std::map<XFtpTask*, int> calls_del;


};

