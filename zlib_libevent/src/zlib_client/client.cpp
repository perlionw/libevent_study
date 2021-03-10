#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#define SPORT 5001
#include <iostream>
using namespace std;
#define FILEPATH "001.bmp"

bufferevent_filter_result filter_out(
	struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, void *ctx)
{
	char data[1024] = { 0 };
	int len = evbuffer_remove(src, data, sizeof(data));
	evbuffer_add(dst, data, len);
	return BEV_OK;
}

void client_read_cb(struct bufferevent *bev, void *ctx)
{
	char data[1024] = { 0 };
	int len = bufferevent_read(bev, data, sizeof(data) - 1);
	if (strcmp(data, "OK") == 0)
	{
		cout << data << endl;

		//开始发送文件出发回调
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
	FILE* fp = (FILE*)ctx;

	if (read_end_flag)
	{
		//判断缓冲是否有数据，如果有数据，获取过滤器绑定的buffer

		bufferevent* be = bufferevent_get_underlying(bev);
		evbuffer* evb = bufferevent_get_output(be);
		//获取输出缓冲区大小
		int len = evbuffer_get_length(evb);
		if (len <= 0)
		{
			//立即清理，如果缓冲区有数据，不会发送
			std::cout << "文件发送成功" << std::endl;
			bufferevent_free(bev);
			return;
		}
		//刷新缓冲区，触发写回调
		bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
		//sbufferevent_trigger(bev, EV_WRITE, 0);
		return;
	}

	if (!fp) return;

	//读取文件
	char data[1024] = {0};
	int len = fread(data, 1, sizeof(data), fp);
	if (len <= 0)
	{
		fclose(fp);
		read_end_flag = true;
		//刷新缓冲区，触发写回调
		bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
		return;
	}

	//发送文件
	bufferevent_write(bev, data, len);
}


void client_event_cb(struct bufferevent *bev, short what, void *ctx)
{
	if (what & BEV_EVENT_CONNECTED)
	{
		cout << "BEV_EVENT_CONNECTED " << what << endl;
		//发送文件名
		bufferevent_write(bev, FILEPATH, strlen(FILEPATH));

		//创建输出过滤,延迟回调
		bufferevent* bev_filter = bufferevent_filter_new(bev, 0, filter_out, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS, 0, 0);
		FILE* fp = fopen(FILEPATH, "rb");
		if (!fp)
		{
			cout << "open file" << FILEPATH << "failed" << endl;
		}

		//设置读取、写入和时间的回调
		bufferevent_setcb(bev_filter, client_read_cb, client_write_cb, client_event_cb, fp);
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