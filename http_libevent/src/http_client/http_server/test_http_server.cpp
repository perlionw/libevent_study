#include <event2/event.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <string.h>
#ifndef _WIN32
#include <signal.h>
#endif
#include <iostream>
#include <string>
using namespace std;

#define WEBROOT "."
#define DEFAULTINDEX "index.html"

void http_cb(struct evhttp_request *req, void *arg)
{
	cout << "http_cb" << endl;
	//1、获取浏览器的请求行
	//uri 
	const char* uri = evhttp_request_get_uri(req);
	cout << "uri: " << uri << endl;

	//请求类型 GET POST
	std::string cmdtype;
	switch (evhttp_request_get_command(req))
	{
	case EVHTTP_REQ_GET:
		cmdtype = "GET";
	case EVHTTP_REQ_POST:
		cmdtype = "POST";
	default:
		break;
	}

	std::cout << "cmdtype: " << cmdtype << endl;
	//消息报头
	evkeyvalq* headers = evhttp_request_get_input_headers(req);
	cout << "===== headers ==== " << endl;

	for (evkeyval* p = headers->tqh_first; p != NULL; p = p->next.tqe_next)
	{
		cout << p->key << ":" << p->value << endl;
	}

	//请求正文（GET为空， POST有表单信息）
	evbuffer* inbuf = evhttp_request_get_input_buffer(req);
	char buf[1024] = { 0 };
	cout << "====== input data =======" << endl;
	while (evbuffer_get_length(inbuf))
	{
		int n = evbuffer_remove(inbuf, buf, sizeof(buf) - 1);
		if (n > 0)
		{
			buf[n] = '\0';
			cout << buf << endl;
		}
	}

	//2 回复浏览器
	//状态行 消息报头 响应正文

	//分析出请求的uri
	//设置根目录的 WEBROOT
	std::string file_path = WEBROOT;
	file_path += uri;

	if (strcmp(uri, "/") == 0)
	{
		//默认加入首页文件
		file_path += DEFAULTINDEX;
	}

	//消息报头
	evkeyvalq* out_head = evhttp_request_get_output_headers(req);
	//要支持 图片 js css 下载 zip 文件
	// 获取文件的后缀
	// ./root/index.html
	int pos = file_path.rfind('.');
	std::string postfix = file_path.substr(pos + 1, file_path.size() - (pos + 1));
	if (postfix == "jpg" || postfix == "gif" || postfix == "png")
	{
		std::string tmp = "image/" + postfix;
		evhttp_add_header(out_head, "Content-Type", tmp.c_str());
	}
	else if (postfix == "zip")
	{
		evhttp_add_header(out_head, "Content-Type", "application/zip");
	}
	else if (postfix == "html")
	{
		evhttp_add_header(out_head, "Content-Type", "text/html;charset=UTF8");
	}
	else if(postfix == "css")
		evhttp_add_header(out_head, "Content-Type", "text/css");

	//读取html文件返回正文
	FILE* fp = fopen(file_path.c_str(), "rb");
	if (!fp)
	{
		evhttp_send_reply(req, HTTP_NOTFOUND, "fopen failed!", 0);
		return;
	}

	evbuffer* outbuf = evhttp_request_get_output_buffer(req);
	for (;;)
	{
		int len = fread(buf, 1, sizeof(buf), fp);
		if (len <= 0) break;
		evbuffer_add(outbuf, buf, len);
	}

	fclose(fp);
	evhttp_send_reply(req, HTTP_OK, "", outbuf);
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
		cout << "event_base_new failed!" << endl;
		return -1;
	}

	//创建http服务器
	//1、创建http上下文
	evhttp* evh = evhttp_new(base);

	//2、绑定端口和IP
	if (evhttp_bind_socket(evh, "0.0.0.0", 8080) != 0)
	{
		cout << "evhttp_bind_socket failed!" << endl;
		return -1;
	}

	//3、设置回调函数
	evhttp_set_gencb(evh, http_cb, 0);

	//事件分发处理
	if (base)
		event_base_dispatch(base);

	if (base)
		event_base_free(base);

	if (evh)
		evhttp_free(evh);

#ifdef _WIN32
	WSACleanup();
#endif

	getchar();
	return 0;
}