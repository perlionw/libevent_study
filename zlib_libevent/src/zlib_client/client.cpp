#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include "zlib/zlib.h"
#include <string.h>
#define SPORT 5001
#include <iostream>
using namespace std;
#define FILEPATH "001.bmp"

struct ClientInfo
{
	FILE* fp = nullptr;
	z_stream* z_output = nullptr;


	~ClientInfo()
	{
		if (fp)
			fclose(fp);
		fp = 0;
		if (z_output)
			deflateEnd(z_output);
		delete z_output;
		z_output = 0;
	}

};

bufferevent_filter_result filter_out(
	struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, void *ctx)
{
	ClientInfo* info = (ClientInfo*)ctx;

	//ѹ���ļ�
	//�����ļ���Ϣ001 ȥ��

	//��ʼѹ���ļ�
	//ȡ��buffer�е�����
	evbuffer_iovec v_in[1];
	int n = evbuffer_peek(src, -1, 0, v_in, 1);
	if (n <= 0)
		return BEV_NEED_MORE; //û������

	//zlib������
	z_stream* p = info->z_output;
	if (!p)
		return BEV_ERROR;

	//zlib �������ݴ�С
	p->avail_in = v_in[0].iov_len;
	//zlib �������ݵ�ַ
	p->next_in = (Byte*)v_in[0].iov_base;

	//��������ռ��С
	evbuffer_iovec v_out[1];
	evbuffer_reserve_space(dst, 4096, v_out, 1);

	//zlib ������ݴ�С
	p->avail_out = v_out[0].iov_len;
	//zlib ������ݵ�ַ
	p->next_out = (Byte*)v_out[0].iov_base;

	//zlibѹ��
	int re = deflate(p, Z_SYNC_FLUSH);
	if (re != Z_OK)
		cerr << "deflate failed!" << endl;

	//ѹ�����˶������ݣ��� source evbuffer ���Ƴ�
	// p->avail_in δ��������ݴ�С
	int nread = v_in[0].iov_len - p->avail_in;

	//ѹ�������ݴ�С ����des evbuffer
	int nwrite = v_out[0].iov_len - p->avail_out;

	//�Ƴ�source evbuffer������
	evbuffer_drain(src, nread);

	//���� des evbuffer
	v_out[0].iov_len = nwrite;
	evbuffer_commit_space(dst, v_out, 1);

	return BEV_OK;
}

void client_read_cb(struct bufferevent *bev, void *ctx)
{
	ClientInfo* info = (ClientInfo*)ctx;
	char data[1024] = { 0 };
	int len = bufferevent_read(bev, data, sizeof(data) - 1);
	if (strcmp(data, "OK") == 0)
	{
		cout << data << endl;
		//��ʼ�����ļ������ص�
		bufferevent_trigger(bev, EV_WRITE, 0);
	}
	else
	{
		bufferevent_free(bev);
	}

	cout << "client_read_cb" << len << endl;

}

void client_write_cb(struct bufferevent *bev, void *ctx)
{
	static bool read_end_flag = false;
	ClientInfo* info = (ClientInfo*)ctx;
	FILE *fp = info->fp;
	if (read_end_flag)
	{
		//�жϻ����Ƿ������ݣ���������ݣ���ȡ�������󶨵�buffer

		bufferevent* be = bufferevent_get_underlying(bev);
		evbuffer* evb = bufferevent_get_output(be);
		//��ȡ�����������С
		int len = evbuffer_get_length(evb);
		if (len <= 0)
		{
			//����������������������ݣ����ᷢ��
			std::cout << "�ļ����ͳɹ�" << std::endl;
			bufferevent_free(bev);
			delete info;
			return;
		}
		//ˢ�»�����������д�ص�
		//bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
		bufferevent_trigger(bev, EV_WRITE, 0);
		return;
	}

	if (!fp) return;

	//��ȡ�ļ�
	char data[1024] = {0};
	int len = fread(data, 1, sizeof(data), fp);
	if (len <= 0)
	{
		read_end_flag = true;
		//ˢ�»�����������д�ص�
		//bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
		bufferevent_trigger(bev, EV_WRITE, 0);
		return;
	}

	//�����ļ�
	bufferevent_write(bev, data, len);
}


void client_event_cb(struct bufferevent *bev, short what, void *ctx)
{
	if (what & BEV_EVENT_CONNECTED)
	{
		cout << "BEV_EVENT_CONNECTED " << what << endl;
		//�����ļ���
		bufferevent_write(bev, FILEPATH, strlen(FILEPATH));
		FILE* fp = fopen(FILEPATH, "rb");
		if (!fp)
		{
			cout << "open file" << FILEPATH << "failed" << endl;
		}

		ClientInfo* info = new ClientInfo();
		info->fp = fp;

		//��ʼ��zlib������ Ĭ��ѹ��
		info->z_output = new z_stream();
		deflateInit(info->z_output, Z_DEFAULT_COMPRESSION);

		//�����������,BEV_OPT_DEFER_CALLBACKS �ӳٻص�
		bufferevent* bev_filter = bufferevent_filter_new(bev, 0, filter_out, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS, 0, info);

		//���ö�ȡ��д���ʱ��Ļص�
		bufferevent_setcb(bev_filter, client_read_cb, client_write_cb, client_event_cb, info);
		bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
	}
}

void Client(event_base* base)
{
	sockaddr_in sin;
	sin.sin_port = htons(5001);
	sin.sin_family = AF_INET;
	evutil_inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr.s_addr);


	bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, 0, 0, client_event_cb, 0);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
	
	bufferevent_socket_connect(bev, (sockaddr*)&sin, sizeof(sin));
}