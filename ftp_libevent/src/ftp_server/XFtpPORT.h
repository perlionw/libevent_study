#pragma once
#include "XFtpTask.h"
#include <vector>
class XFtpPORT :
	public XFtpTask
{
public:

	virtual void Parse(std::string type, std::string data);
};

