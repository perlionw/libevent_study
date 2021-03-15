#include "XFtpRETR.h"
#include <event2/bufferevent.h>
#include <event2/event.h>
#include<iostream>
using namespace std;
void XFtpRETR::Parse(std::string type, std::string msg)
{
	//文件名
	int pos = msg.rfind(" ") + 1;
	std::string file_name = msg.substr(pos, msg.size() - pos - 2);
	std::string path = cmdTask->rootDir;
	path += cmdTask->curDir;
	path += file_name;
	fp = fopen(path.c_str(), "rb");
	if (fp)
	{
		//连接数据通道
		ConnectPort();

		//发送开始下载的指令
		ResCMD("150 File OK\r\n");

		//触发写入事件
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
	//如果对方网络断掉，或者机器死机有可能收不到BEV_EVENT_EOF数据
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
