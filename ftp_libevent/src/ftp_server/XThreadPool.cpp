#include "XThreadPool.h"
#include "XThread.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
	using namespace std::literals::chrono_literals;
#endif

void XThreadPool::Init(int threadCount)
{
	this->threadCount = threadCount;
	this->lastThread = -1;
	for (int i = 0; i < threadCount; ++i)
	{
		XThread* thread = new XThread();
		thread->id = i + 1;
		std::cout << "Create thread " << std::endl;
		thread->Start();
		threads.push_back(thread);
		//std::this_thread::sleep_for(10ms);
		std::this_thread::sleep_for(std::chrono::microseconds(10));
	}
}

void XThreadPool::Dispatch(XTask* task)
{
	if (!task) return;
	int tid = (lastThread + 1) % threadCount;
	lastThread = tid;
	XThread *t = threads[tid];
	t->AddTask(task);
	t->Activate();
}
