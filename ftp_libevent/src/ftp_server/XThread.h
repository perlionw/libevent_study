#pragma once
#include <event2/event.h>
#include <list>
#include <mutex>
class XTask;
class XThread
{

private:

	//��װ�̣߳���ʼ��event_base�͹ܵ������¼����ڼ���
	bool Setup();

	//�߳���ں���
	void Main();
public:

	//�����߳�
	void Start();


	// �̼߳���
	void Activate();


	//�յ����̷߳����ļ�����Ϣ���̳߳طַ���
	void Notify(evutil_socket_t fd, short which);

	//��Ӵ��������һ���߳̿��Դ��������񣬹���һ��event_base
	void AddTask(XTask* t);


	XThread() {}
	~XThread() {}

	//�̱߳��
	int id = 0;
private:
	
	int notify_send_fd = 0;
	struct event_base* base = 0;

	//�����б�
	std::list<XTask*> tasks;

	//�̰߳�ȫ����
	std::mutex tasks_metux;

};

