
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>
using namespace std;
#define SPORT 5001

static bufferevent_filter_result filter_in(
	struct evbuffer *src, 
	struct evbuffer *dst, 
	ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, 
	void *ctx)
{
	std::cout << "filter_in" << std::endl;
	//1�����ܿͻ��˷��͵��ļ���
	char data[1024] = { 0 };
	int len = evbuffer_remove(src, data, sizeof(data) - 1); //��ȡ�����buffer

	//2������ ���ص�
	evbuffer_add(dst, data, len);
	return BEV_OK;
}


void read_cb(struct bufferevent *bev, void *ctx)
{
	std::cout << "read_cb" << std::endl;
	char data[1024] = { 0 };
	bufferevent_read(bev, data, sizeof(data) - 1);
	std::cout << data << std::endl;
}


void event_cb(struct bufferevent *bev, short what, void *ctx)
{
	cout << "server event cb " << endl;
	if (what & BEV_EVENT_EOF)
	{
		cout << "server event BEV_EVENT_EOF" << endl;
		bufferevent_free(bev);
	}

}

static void listen_cb(struct evconnlistener * e, evutil_socket_t s, struct sockaddr * a, int socklen, void *arg)
{
	cout << "listen_cb" << endl;

	event_base* base = (event_base*)arg;

	//1�� ����һ��bufferevent����ͨ��
	bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);

	//2�� ��ӹ����� �����ûص�
	bufferevent* bev_filter = bufferevent_filter_new(bev,
		filter_in, //������˺���
		0,  //������˺���
		BEV_OPT_CLOSE_ON_FREE, //�ر�filter ͬʱ����bufferevnet
		0, //����ص�
		nullptr
	);

	//3�� δ���������ûص� ��ȡ�¼�
	bufferevent_setcb(bev_filter, read_cb, 0, event_cb, nullptr);
	bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
}

void Server(event_base* base)
{
	//�����˿�
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);

	evconnlistener* ev = evconnlistener_new_bind(base,  //libevent ������
		listen_cb, //���յ����ӵĻص�����
		base, //�ص�������ȡ���û����� arg
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, //��ַ���ã� evconnlister �ر�ͬʱ�ر�socket
		10, //���Ӷ��д�С�� ��Ӧ listen ����
		(sockaddr*)&sin,  //�󶨵ĵ�ַ�Ͷ˿�
		sizeof(sin));
}