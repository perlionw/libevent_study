
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <string>
#include <iostream>
#include <string.h>
#include "zlib/zlib.h"
using namespace std;
#define SPORT 5001

struct FileInfo
{
	FILE* fp = nullptr;
	z_stream* p = 0;
	
	~FileInfo()
	{
		if (fp)
			fclose(fp);
		fp = 0;
		if (p)
			inflateEnd(p);
		delete p;
		p = 0;
	}	
};

static bufferevent_filter_result filter_in(
	struct evbuffer *src, 
	struct evbuffer *dst, 
	ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, 
	void *ctx)
{

	FileInfo* info = (FileInfo*)ctx;

	static bool get_file_name_flag = false;
	std::cout << "filter_in" << std::endl;
	//1�����ܿͻ��˷��͵��ļ���
	if (!get_file_name_flag)
	{
		char data[1024] = { 0 };
		int len = evbuffer_remove(src, data, sizeof(data) - 1); //��ȡ�����buffer
		std::cout << len << std::endl;
		//2������ ���ص�
		evbuffer_add(dst, data, len);
		get_file_name_flag = true;
		return BEV_OK;
	}

	//��ѹ
	evbuffer_iovec v_in[1];
	//��ȡ���� ��������
	int n = evbuffer_peek(src, -1, NULL, v_in, 1);
	if (n <= 0)
		return BEV_NEED_MORE;

	//��ѹ������
	z_stream* p = info->p;

	//zlib �������ݴ�С
	p->avail_in = v_in[0].iov_len;
	//zlib �������ݵ�ַ
	p->next_in = (Byte*)v_in[0].iov_base;

	//��������ռ��С
	evbuffer_iovec v_out[1];
	evbuffer_reserve_space(dst, 4096, v_out, 1);
	
	//������ݴ�С
	p->avail_out = v_out[0].iov_len;
	//������ݵ�ַ
	p->next_out = (Byte*)v_out[0].iov_base;

	int re = inflate(p, Z_SYNC_FLUSH);

	if (re != Z_OK)
	{
		cerr << "inflate failed!!" << endl;
	}

	//��ѹ���˶������ݣ���source evbuffer���Ƴ�
	//p->avail_in δ��������ݴ�С
	int nread = v_in[0].iov_len - p->avail_in;

	//��ѹ�����ݴ�С ���� des evbuffer
	//p->avail_out ʣ��ռ��С
	int nwrite = v_out[0].iov_len - p->avail_out;

	//�Ƴ�source evbuffer������
	evbuffer_drain(src, nread);

	
	//���� des evbuffer
	v_out[0].iov_len = nwrite;
	evbuffer_commit_space(dst, v_out, 1);

	return BEV_OK;
}


void read_cb(struct bufferevent *bev, void *ctx)
{
	FileInfo* info = (FileInfo*)ctx;
	static bool get_file_name_flag = false;

	if (!get_file_name_flag)
	{
		//�����ļ���
		char data[1024] = { 0 };
		bufferevent_read(bev, data, sizeof(data) - 1);
		std::cout << data << std::endl;
		
		string out = "out/";
		out += data;
		
		//��д���ļ�
		info->fp = fopen(out.c_str(), "wb");
		if (!info->fp)
		{
			cout << "server open " << out << "failed" << endl;
			return;
		}
		get_file_name_flag = true;
		// �ظ�OK
		bufferevent_write(bev, "OK", 2);
		return;
	}

	do
	{
		//д���ļ�
		char data[1024] = { 0 };
		int len = bufferevent_read(bev, data, sizeof(data));
		if (len >= 0)
		{
			fwrite(data, 1, len, info->fp);
			fflush(info->fp);
		}
	} while (evbuffer_get_length(bufferevent_get_input(bev)) > 0);

	cout << "д���ļ��ɹ�������" << endl;
}


void event_cb(struct bufferevent *bev, short what, void *ctx)
{
	FileInfo* info = (FileInfo*)ctx;
	cout << "server event cb " << endl;
	if (what & BEV_EVENT_EOF)
	{
		cout << "server event BEV_EVENT_EOF" << endl;
		delete info;
		bufferevent_free(bev);
	}
	else if (what & BEV_EVENT_ERROR)
	{
		delete info;
		cout << "server event BEV_EVENT_ERROR" << endl;
		bufferevent_free(bev);
	}
}

static void listen_cb(struct evconnlistener * e, evutil_socket_t s, struct sockaddr * a, int socklen, void *arg)
{
	cout << "listen_cb" << endl;

	event_base* base = (event_base*)arg;

	//1�� ����һ��bufferevent����ͨ��
	bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);


	FileInfo* info = new FileInfo;

	info->p = new z_stream();
	inflateInit(info->p);

	//2�� ��ӹ����� �����ûص�
	bufferevent* bev_filter = bufferevent_filter_new(bev,
		filter_in, //������˺���
		0,  //������˺���
		BEV_OPT_CLOSE_ON_FREE, //�ر�filter ͬʱ����bufferevnet
		0, //����ص�
		info
	);


	//3�� δ���������ûص� ��ȡ�¼�
	bufferevent_setcb(bev_filter, read_cb, 0, event_cb, info);
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