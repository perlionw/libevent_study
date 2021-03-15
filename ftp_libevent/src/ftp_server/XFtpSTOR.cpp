#include "XFtpSTOR.h"
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <iostream>
using namespace std;

void XFtpSTOR::Parse(std::string type, std::string msg)
{
	//�ļ���
	int pos = msg.rfind(" ") + 1;
	std::string filename = msg.substr(pos, msg.size() - pos - 2);
	std::string path = cmdTask->rootDir;
	path += cmdTask->curDir;
	path += filename;

	fp = fopen(path.c_str(), "wb");
	if(fp)
	{

		//��������ͨ��
		ConnectPort();
		
		//���Ϳ�ʼ�����ļ���ָ��
		ResCMD("125 File OK\r\n");

		//������ȡ�¼�
		//bufferevent_trigger(bev, EV_READ, 0);
	}
	else
	{
		ResCMD("450 file open failed!\r\n");
	}
}

void XFtpSTOR::Read(struct bufferevent *bev)
{
	if (!fp)return;
	for (;;)
	{
		int len = bufferevent_read(bev, buf, sizeof(buf));
		if (len <= 0)
			return;

		int size = fwrite(buf, 1, len, fp);
	}
}

void XFtpSTOR::Event(struct bufferevent *bev, short what)
{
	//����Է�����ϵ������߻��������п����ղ���BEV_EVENT_EOF����
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << "XFtpSTOR BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << endl;
		Close();
		if (fp)
		{
			fclose(fp);
			fp = 0;
		}
		ResCMD("226 Transfer complete\r\n");
	}
	else if (what&BEV_EVENT_CONNECTED)
	{
		cout << "XFtpSTOR BEV_EVENT_CONNECTED" << endl;
	}
}