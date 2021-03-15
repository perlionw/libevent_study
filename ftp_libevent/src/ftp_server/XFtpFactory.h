#pragma once

class XTask;
class XFtpFactory
{

public:
	static XFtpFactory* Get()
	{
		static XFtpFactory f;
		return &f;
	}

	XTask* CreateTask();
private:
	XFtpFactory() {}
	~XFtpFactory() {}
};

