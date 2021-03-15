#include "XFtpPORT.h"
#include <string>
void XFtpPORT::Parse(std::string type, std::string msg)
{
	//PORT 127,0,0,1,219,69\r\n
	//PORT n1,n2,n3,n4,n5,n6\r\n
	//port = n5*256 + n6

	//只获取ip和端口， 不连接
	std::vector<std::string> vals;
	std::string temp = "";

	for (int i = 5; i < msg.size(); ++i)
	{
		if (msg[i] == ',' || msg[i] == '\r')
		{
			vals.push_back(temp);
			temp = "";
			continue;
		}
		temp += msg[i];
	}

	if (vals.size() != 6)
	{
		//PORT 格式有误
		ResCMD("501 Syntax error in parameters or arguments.");
		return;
	}

	ip = vals[0] + "." + vals[1] + "." + vals[2] + "." + vals[3];
	port = atoi(vals[4].c_str()) * 256 + atoi(vals[5].c_str());

	std::cout << "PORT ip is " << ip << std::endl;
	std::cout << "PORT port is " << port << std::endl;

	ResCMD("200 PORT command successful.\r\n");
}
