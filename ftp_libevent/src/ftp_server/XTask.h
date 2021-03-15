#pragma once
class XTask
{

public:
	virtual bool Init() = 0;

	int sock = 0;
	int thread_id = 0;
	struct event_base *base = 0;
};