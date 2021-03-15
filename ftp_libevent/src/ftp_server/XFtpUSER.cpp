#include "XFtpUSER.h"
#include <string>
void XFtpUSER::Parse(std::string type, std::string data)
{
	std::cout << "XFtpUSER::Parse " << type << " " << data << std::endl;
	ResCMD("230 Login successful.\r\n");
}

XFtpUSER::XFtpUSER()
{

}

XFtpUSER::~XFtpUSER()
{

}
