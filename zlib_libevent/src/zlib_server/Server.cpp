
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <string>
#include <iostream>
#include <string.h>
using namespace std;
#define SPORT 5001

struct FileInfo
{
	int temp;
	FILE* fp = nullptr;
};

static bufferevent_filter_result filter_in(
	struct evbuffer *src, 
	struct evbuffer *dst, 
	ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, 
	void *ctx)
{
	std::cout << "filter_in" << std::endl;
	//1、接受客户端发送的文件名
	char data[1024] = { 0 };
	int len = evbuffer_remove(src, data, sizeof(data) - 1); //读取并清空buffer
	std::cout << len << std::endl;
	//2、触发 读回调
	evbuffer_add(dst, data, len);
	return BEV_OK;
}


void read_cb(struct bufferevent *bev, void *ctx)
{
	FileInfo* info = (FileInfo*)ctx;
	static bool get_file_name_flag = false;

	if (!get_file_name_flag)
	{
		//接受文件名
		char data[1024] = { 0 };
		bufferevent_read(bev, data, sizeof(data) - 1);
		std::cout << data << std::endl;
		
		string out = "out/";
		out += data;
		
		//打开写入文件
		info->fp = fopen(out.c_str(), "wb");
		if (!info->fp)
		{
			cout << "server open " << out << "failed" << endl;
			return;
		}
		get_file_name_flag = true;
		// 回复OK
		bufferevent_write(bev, "OK", 2);
		return;
	}

	do
	{
		//写入文件
		char data[1024] = { 0 };
		int len = bufferevent_read(bev, data, sizeof(data));
		if (len >= 0)
		{
			fwrite(data, 1, len, info->fp);
			fflush(info->fp);
		}
	} while (evbuffer_get_length(bufferevent_get_input(bev)) > 0);

	cout << "写入文件成功！！！" << endl;
}


void event_cb(struct bufferevent *bev, short what, void *ctx)
{
	FileInfo* info = (FileInfo*)ctx;
	cout << "server event cb " << endl;
	if (what & BEV_EVENT_EOF)
	{
		cout << "server event BEV_EVENT_EOF" << endl;
		if (info->fp)
		{
			fclose(info->fp);
			info->fp = 0;
		}
		delete info;
		bufferevent_free(bev);
	}
	else if (what & BEV_EVENT_ERROR)
	{
		if (info->fp)
		{
			fclose(info->fp);
			info->fp = 0;
		}
		delete info;
		cout << "server event BEV_EVENT_ERROR" << endl;
		bufferevent_free(bev);
	}
}

static void listen_cb(struct evconnlistener * e, evutil_socket_t s, struct sockaddr * a, int socklen, void *arg)
{
	cout << "listen_cb" << endl;

	event_base* base = (event_base*)arg;

	//1、 创建一个bufferevent用来通信
	bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);

	//2、 添加过滤器 并设置回调
	bufferevent* bev_filter = bufferevent_filter_new(bev,
		filter_in, //输入过滤函数
		0,  //输出过滤函数
		BEV_OPT_CLOSE_ON_FREE, //关闭filter 同时管理bufferevnet
		0, //清理回调
		nullptr
	);

	FileInfo* info = new FileInfo;

	//3、 未过滤器设置回调 读取事件
	bufferevent_setcb(bev_filter, read_cb, 0, event_cb, info);
	bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
}

void Server(event_base* base)
{
	//监听端口
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);

	evconnlistener* ev = evconnlistener_new_bind(base,  //libevent 上下文
		listen_cb, //接收到连接的回调函数
		base, //回调函数获取的用户参数 arg
		LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, //地址重用， evconnlister 关闭同时关闭socket
		10, //连接队列大小， 对应 listen 函数
		(sockaddr*)&sin,  //绑定的地址和端口
		sizeof(sin));
}