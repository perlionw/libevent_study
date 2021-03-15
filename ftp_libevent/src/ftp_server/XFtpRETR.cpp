#include "XFtpRETR.h"
#include <event2/bufferevent.h>
#include <event2/event.h>
#include<iostream>
using namespace std;
void XFtpRETR::Parse(std::string type, std::string msg)
{
	//�ļ���
	int pos = msg.rfind(" ") + 1;
	std::string file_name = msg.substr(pos, msg.size() - pos - 2);
	std::string path = cmdTask->rootDir;
	path += cmdTask->curDir;
	path += file_name;
	fp = fopen(path.c_str(), "rb");
	if (fp)
	{
		//��������ͨ��
		ConnectPort();

		//���Ϳ�ʼ���ص�ָ��
		ResCMD("150 File OK\r\n");

		//����д���¼�
		bufferevent_trigger(bev, EV_WRITE, 0);
	}
	else
	{
		ResCMD("450 file open failed!\r\n");
	}
}

void XFtpRETR::Write(struct bufferevent *bev)
{
	if (!fp)return;
	int len = fread(buf, 1, sizeof(buf), fp);
	if (len <= 0)
	{
		fclose(fp);
		fp = 0;
		ResCMD("226 Transfer Complete\r\n");
		Close();
		return;
	}
	
	Send(buf, len);
}

void XFtpRETR::Event(struct bufferevent *bev, short what)
{
	//����Է�����ϵ������߻��������п����ղ���BEV_EVENT_EOF����
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << "BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << endl;
		Close();
		if (fp)
		{
			fclose(fp);
			fp = 0;
		}
	}
	else if (what&BEV_EVENT_CONNECTED)
	{
		cout << "XFtpRETR BEV_EVENT_CONNECTED" << endl;
	}
}
