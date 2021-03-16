#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <string>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif
#include <iostream>
using namespace std;

void http_client_cb(struct evhttp_request *req, void *arg)
{
	cout << "http_client_cb" << endl;
	bufferevent* bev = (bufferevent*)arg;

	//服务端响应错误
	if (req == NULL)
	{
		int err_code = EVUTIL_SOCKET_ERROR();
		cout << "socket error:" << evutil_socket_error_to_string(err_code) << endl;
		return;
	}

	//获取uri
	const char* path = evhttp_request_get_uri(req);
	cout << "request path is " << path << endl;

	//获取返回的code 200 404
	cout << "Response: " << evhttp_request_get_response_code(req) << " " <<
		evhttp_request_get_response_code_line(req) << endl;

	char buf[1024] = { 0 };
	evbuffer* inbuf = evhttp_request_get_input_buffer(req);
	for (;;)
	{
		int len = evbuffer_remove(inbuf, buf, sizeof(buf) - 1);
		if (len <= 0) break;
		buf[len] = 0;
		cout << buf << flush;
	}
}

int main()
{
#ifdef _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return 1;
#endif

	//创建上下文
	event_base* base = event_base_new();
	if (!base)
	{
		std::cout << "event_base_new failed!" << std::endl;
		return -1;
	}

	//生成请求消息 GET
	std::string http_url = "http://ffmpeg.club/index.html?id=1";

	//分析url地址
	//uri
	evhttp_uri *uri = evhttp_uri_parse(http_url.c_str());

	//http https
	const char* scheme = evhttp_uri_get_scheme(uri);
	if (!scheme)
	{
		cerr << "scheme is null!" << endl;
		return -1;
	}

	cout << "scheme is: " << scheme << endl;

	//端口
	int port = evhttp_uri_get_port(uri);
	if (port < 0)
	{
		if (strcmp(scheme, "http") == 0)
			port = 80;
	}
	
	cout << "port: " << port << endl;

	//主机
	const char* host = evhttp_uri_get_host(uri);
	if (!host)
	{
		cerr << "host is null " << endl;
		return -1;
	}
	cout << "host: " << host << endl;

	const char* path = evhttp_uri_get_path(uri);
	if (!path || strlen(path) == 0)
		path = "/";
	if (path)
		cout << "path is " << path << endl;

	//提取url中的参数
	const char* query = evhttp_uri_get_query(uri);
	if (query)
		cout << "query is " << query << endl;
	else
		cout << "query is NULL" << endl;

	//buffervent 连接http服务器
	bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	evhttp_connection* evcon = evhttp_connection_base_bufferevent_new(base, NULL, bev, host, port);

	//http_client 请求回调函数设置
	evhttp_request* req = evhttp_request_new(http_client_cb, bev);

	//设置请求的head 消息报头 信息
	evkeyvalq* out_head = evhttp_request_get_output_headers(req);
	evhttp_add_header(out_head, "Host", host);

	//发起请求
	evhttp_make_request(evcon, req, EVHTTP_REQ_GET, path);

	//事件分发处理
	if (base)
		event_base_dispatch(base);
	if (base)
		event_base_free(base);

#ifdef _WIN32
	WSACleanup();
#endif
	getchar();
	return 0;
}