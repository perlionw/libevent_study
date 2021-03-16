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

	//�������Ӧ����
	if (req == NULL)
	{
		int err_code = EVUTIL_SOCKET_ERROR();
		cout << "socket error:" << evutil_socket_error_to_string(err_code) << endl;
		return;
	}

	//��ȡuri
	const char* path = evhttp_request_get_uri(req);
	cout << "request path is " << path << endl;

	//��ȡ���ص�code 200 404
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

	//����������
	event_base* base = event_base_new();
	if (!base)
	{
		std::cout << "event_base_new failed!" << std::endl;
		return -1;
	}

	//����������Ϣ GET
	std::string http_url = "http://ffmpeg.club/index.html?id=1";

	//����url��ַ
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

	//�˿�
	int port = evhttp_uri_get_port(uri);
	if (port < 0)
	{
		if (strcmp(scheme, "http") == 0)
			port = 80;
	}
	
	cout << "port: " << port << endl;

	//����
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

	//��ȡurl�еĲ���
	const char* query = evhttp_uri_get_query(uri);
	if (query)
		cout << "query is " << query << endl;
	else
		cout << "query is NULL" << endl;

	//buffervent ����http������
	bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	evhttp_connection* evcon = evhttp_connection_base_bufferevent_new(base, NULL, bev, host, port);

	//http_client ����ص���������
	evhttp_request* req = evhttp_request_new(http_client_cb, bev);

	//���������head ��Ϣ��ͷ ��Ϣ
	evkeyvalq* out_head = evhttp_request_get_output_headers(req);
	evhttp_add_header(out_head, "Host", host);

	//��������
	evhttp_make_request(evcon, req, EVHTTP_REQ_GET, path);

	//�¼��ַ�����
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