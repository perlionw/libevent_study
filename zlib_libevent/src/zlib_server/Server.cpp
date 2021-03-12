
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
	//1、接受客户端发送的文件名
	if (!get_file_name_flag)
	{
		char data[1024] = { 0 };
		int len = evbuffer_remove(src, data, sizeof(data) - 1); //读取并清空buffer
		std::cout << len << std::endl;
		//2、触发 读回调
		evbuffer_add(dst, data, len);
		get_file_name_flag = true;
		return BEV_OK;
	}

	//解压
	evbuffer_iovec v_in[1];
	//读取数据 不清理缓冲
	int n = evbuffer_peek(src, -1, NULL, v_in, 1);
	if (n <= 0)
		return BEV_NEED_MORE;

	//解压上下文
	z_stream* p = info->p;

	//zlib 输入数据大小
	p->avail_in = v_in[0].iov_len;
	//zlib 输入数据地址
	p->next_in = (Byte*)v_in[0].iov_base;

	//申请输出空间大小
	evbuffer_iovec v_out[1];
	evbuffer_reserve_space(dst, 4096, v_out, 1);
	
	//输出数据大小
	p->avail_out = v_out[0].iov_len;
	//输出数据地址
	p->next_out = (Byte*)v_out[0].iov_base;

	int re = inflate(p, Z_SYNC_FLUSH);

	if (re != Z_OK)
	{
		cerr << "inflate failed!!" << endl;
	}

	//解压用了多少数据，从source evbuffer中移除
	//p->avail_in 未处理的数据大小
	int nread = v_in[0].iov_len - p->avail_in;

	//解压后数据大小 传入 des evbuffer
	//p->avail_out 剩余空间大小
	int nwrite = v_out[0].iov_len - p->avail_out;

	//移除source evbuffer中数据
	evbuffer_drain(src, nread);

	
	//传入 des evbuffer
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

	//1、 创建一个bufferevent用来通信
	bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);


	FileInfo* info = new FileInfo;

	info->p = new z_stream();
	inflateInit(info->p);

	//2、 添加过滤器 并设置回调
	bufferevent* bev_filter = bufferevent_filter_new(bev,
		filter_in, //输入过滤函数
		0,  //输出过滤函数
		BEV_OPT_CLOSE_ON_FREE, //关闭filter 同时管理bufferevnet
		0, //清理回调
		info
	);


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