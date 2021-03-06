#include "XFtpSTOR.h"
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <iostream>
using namespace std;

void XFtpSTOR::Parse(std::string type, std::string msg)
{
	//文件名
	int pos = msg.rfind(" ") + 1;
	std::string filename = msg.substr(pos, msg.size() - pos - 2);
	std::string path = cmdTask->rootDir;
	path += cmdTask->curDir;
	path += filename;

	fp = fopen(path.c_str(), "wb");
	if(fp)
	{

		//连接数据通道
		ConnectPort();
		
		//发送开始接受文件的指令
		ResCMD("125 File OK\r\n");

		//触发读取事件
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
		{
			return;
		}
			
		int size = fwrite(buf, 1, len, fp);
	}
}

void XFtpSTOR::Event(struct bufferevent *bev, short what)
{
	//如果对方网络断掉，或者机器死机有可能收不到BEV_EVENT_EOF数据
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << "XFtpSTOR BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << endl;
		//关闭文件句柄
		if (fp)
		{
			fclose(fp);
			fp = 0;
		}
		//此处不会关闭文件句柄
		Close();
		ResCMD("226 Transfer complete\r\n");
	}
	else if (what&BEV_EVENT_CONNECTED)
	{
		cout << "XFtpSTOR BEV_EVENT_CONNECTED" << endl;
	}
}
