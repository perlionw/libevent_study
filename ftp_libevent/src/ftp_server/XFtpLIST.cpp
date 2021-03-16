#include "XFtpLIST.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#ifdef _WIN32
#include <io.h>
#endif


void XFtpLIST::Parse(std::string type, std::string msg)
{
	std::string resmsg = "";

	if (type == "PWD")
	{
		//257 "/" is current directory
		resmsg = "257 \"";
		resmsg += cmdTask->curDir;
		resmsg += "\" is current directory.\r\n";

		ResCMD(resmsg);
	}
	else if (type == "LIST")
	{
		//1 ��������ͨ�� 
		//2 150
		//3 ����Ŀ¼����ͨ��
		//4 �������226
		//5 �ر�����
		//-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg\r\n
		//1�� ��������ͨ��
		ConnectPort();

		//2��1502 150
		ResCMD("150 Here comes the directory listing.\r\n");

		//std::string list_data = "-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg\r\n";
		std::string list_data = GetListData(cmdTask->rootDir + cmdTask->curDir);
		//3������ͨ������
		Send(list_data);
	}
	else if(type == "CWD") //�л�Ŀ¼
	{
		if (msg.empty())
		{
			std::cout << "CWD Ŀ¼Ϊ��" << std::endl;
			return;
		}
		//ȥ�������е�·��
		//CWD test\r\n
		int pos = msg.rfind(" ") + 1;
		//ȥ����β��\r\n
		std::string path = msg.substr(pos, msg.size() - pos - 2);
		if (path[0] == '/') //����·��
			cmdTask->curDir = path;
		else
		{
			if (cmdTask->curDir[cmdTask->curDir.size() - 1] != '/')
				cmdTask->curDir += "/";
			cmdTask->curDir += path + "/";
		}

		// /test/
		ResCMD("250 Directory succes chanaged.\r\n");
	}
	else if (type == "CDUP") //�ص��ϲ�Ŀ¼
	{
		if (msg.empty())
		{
			std::cout << "CDUP Ŀ¼Ϊ��" << std::endl;
			return;
		}

		std::string path = cmdTask->curDir;
		//ͳһȥ����β�� /
		if (path[path.size() - 1] == '/')
		{
			path = path.substr(0, path.size() - 1);
		}

		int pos = path.rfind("/");
		path = path.substr(0, pos + 1);
		cmdTask->curDir = path;
		ResCMD("250 Directory succes chanaged.\r\n");
	}
}

void XFtpLIST::Write(struct bufferevent* bev)
{
	//4�� 226 Transfer complete �������
	ResCMD("226 Transfer complete\r\n");
	//5�� �ر�����
	Close();
}

void XFtpLIST::Event(struct bufferevent*bev, short what)
{
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		std::cout << "BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << std::endl;
		Close();
	}
	else if (what & BEV_EVENT_CONNECTED)
	{
		std::cout << "XFtpLIST BEV_EVENT_CONNECTED" << std::endl;
	}
}

std::string XFtpLIST::GetListData(std::string path)
{
	std::string data = "";

#ifdef _WIN32
	//�洢�ļ���Ϣ
	_finddata_t file;

	//Ŀ¼������
	path += "/*.*";
	intptr_t dir = _findfirst(path.c_str(), &file);
	if (dir < 0)
		return data;

	do
	{
		std::string tmp = "";
		if (file.attrib & _A_SUBDIR)
		{
			if (strcmp(file.name, ".") == 0 || strcmp(file.name, "..") == 0)
				continue;
			tmp = "drwxrwxrwx 1 root group ";
		}
		else
			tmp = "-rwxrwxrwx 1 root group ";

		//�ļ���С
		char buf[1024] = { 0 };
		sprintf(buf, "%u ", file.size);
		tmp += buf;

		//����ʱ��
		strftime(buf, sizeof(buf) - 1, "%b %d %H:%M ", localtime(&file.time_write));
		tmp += buf;
		tmp += file.name;
		tmp += "\r\n";
		data += tmp;

	} while (_findnext(dir, &file) == 0);
#else
	std::string cmd = "ls -l ";
	cmd += path;
	std::cout << "popen: " << cmd << std::endl;
	FILE* f = popen(cmd.c_str(), "r");
	if (!f)
		return data;
	char buffer[1024] = { 0 };
	for (;;)
	{
		int len = fread(buffer, 1, sizeof(buffer) - 1, f);
		if (len <= 0)break;
		buffer[len] = '\0';
		data += buffer;
	}
	pclose(f);
#endif

	return data;
}

