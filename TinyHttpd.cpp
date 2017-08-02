// -----------------------------------------------------------------------*/
//
//文件名：TinyHttpd.cpp
//创建人：MasticZhang
//创建时间：2017/7/19 10：47
//功能描述：支持处理cgi的一个多线程 web server ......
//
// -----------------------------------------------------------------------*/

#include <stdio.h>
#include <WinSock2.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tchar.h>

#include "WinCGI.h"
#include "ThreadPool.h"
using namespace MasticThreadPool;

#pragma comment(lib,"wsock32.lib")
#pragma warning(disable:4267)

#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server:Mastic—Tinthttp /0.1.0\r\n"

class MasticTinyHttpd
{
public:
	typedef struct SocketContext {
		SOCKET socket_client;
		SocketContext() :socket_client(-1) {}
	}SOCKET_CONTEXT,*PSOCKET_CONTEXT;



	// -----------------------------------------------------------------------*/
	//函数方法：get_line
	//作用：读取一行http报文
	// -----------------------------------------------------------------------*/
	int get_line(SOCKET sock, char *buf, int bufsize) {
		int n,loop_i = 0;
		char c = '\0';

		while (loop_i < bufsize - 1 && c != '\n') {
			n = recv(sock, &c, 1, 0);
			if (n > 0) {
				if (c == '\r') {
					n = recv(sock, &c, 1, MSG_PEEK);
					if (n > 0 && c == '\n') {
						recv(sock, &c, 1, 0);
					}
					else {
						c = '\n';
					}
				}
				buf[loop_i++] = c;
			}
			else {
				c = '\n';
			}
		}//while end
		buf[loop_i] = '\0';

		return loop_i;
	}





	// -----------------------------------------------------------------------*/
	//函数方法：accept_request
	//作用：处理从套接字上监听到的http请求
	// -----------------------------------------------------------------------*/
	void accept_request(SOCKET_CONTEXT &client_context) {
		printf("Thread ID [%u] accept request\n",(unsigned int)::GetCurrentThreadId());

		int cgi = 0;
		SOCKET client = client_context.socket_client;

		char buf[1024]{ 0 };
		int numchars = get_line(client,buf,sizeof(buf));

		//获取http请求的方法名
		int loop_i = 0, loop_header = 0;
		char method[255]{ 0 };
		while (loop_i < numchars && !ISspace(buf[loop_i]) && loop_header < sizeof(method) - 1) {
			method[loop_header++] = buf[loop_i++];
		}
		method[loop_header] = '\0';

		//只处理get和post 不处理其他的
		if (_stricmp(method, "GET") && _stricmp(method, "POST")) {
			if (numchars > 0) {
				discardheaders(client);
			}
			unimplemented(client);
			closesocket(client);
			return;
		}

		//POST请求
		if (0 == _stricmp(method, "POST")) {
			cgi = 1;
		
		}

		//获取url
		char url[255]{ 0 };
		loop_i = 0;
		while (ISspace(buf[loop_header]) && loop_header < sizeof(buf)) {
			loop_header++;
		}
		while (loop_i < sizeof(url) && !ISspace(buf[loop_header]) && loop_header < sizeof(buf)) {
			url[loop_i++] = buf[loop_header++];
		}
		url[loop_i] = '\0';

		//GET请求
		char *query_string = nullptr;
		if (0 == _stricmp(method, "GET")) {
			query_string = url;
			while (*query_string != '?' && *query_string != '\0') {
				query_string++;
			}
			if (*query_string == '?') {
				cgi = 1;
				*query_string++ = '\0';
			}
		}

		//生成路径
		char path[512]{ 0 };
		sprintf_s(path,512,"htdocs%s",url);

		if (path[strlen(path) - 1] == '/') {
			strcat_s(path,512,"index.html");
		}

		struct stat st;
		if (-1 == stat(path, &st)) {
			if (numchars < 0) {
				discardheaders(client);
			}
			not_found(client);
		}
		else {
			//文件夹属性
			if ((st.st_mode & S_IFMT) == S_IFDIR) {
				strcat_s(path, 512, "index.html");
			}

			//可执行属性
			if (st.st_mode & S_IEXEC) {
				cgi = 1;
			}

			if (!cgi) {
				serve_file(client, path);
			}
			else {
				execute_cgi(client, path, method, query_string);
			}
		}
		closesocket(client);
	}




	// -----------------------------------------------------------------------*/
	//函数方法：serve_file
	//作用：静态处理情况
	// -----------------------------------------------------------------------*/
	void serve_file(SOCKET sock,const char *filename) {
		FILE *resource = nullptr;
		discardheaders(sock);
	
		fopen_s(&resource, filename, "r");
		if (resource == nullptr) {
			not_found(sock);
		}
		else {
			headers(sock);
			cat(sock, resource);
		}
		fclose(resource);
	}




	// -----------------------------------------------------------------------*/
	//函数方法：execute_cgi
	//作用：处理动态解析执行CGI脚本的情况
	// -----------------------------------------------------------------------*/
	void execute_cgi(SOCKET sock, const char *path, const char *method, const char *query_string) {
		
		int content_length = -1;

		if (0 == _stricmp(method, "GET")) {
			discardheaders(sock);
		}
		else {//POST方法
			char buf[1024]{0};
			buf[0] = 'A'; buf[1] = '\0';
			int numchars = get_line(sock, buf, sizeof(buf));

			while (numchars > 0 && strcmp("\n", buf)) {
				buf[15] = '\0';
				if (0 == _stricmp(buf, "Content_Length:")) {
					content_length = atoi(&buf[16]);
				}
				numchars = get_line(sock, buf, sizeof(buf));
			}//while end
		
			if (-1 == content_length) {
				bad_request(sock);
				return;
			}
		}

		WinCGI cgi;
		char c;
		if (!cgi.Exec(path, query_string)) {
			bad_request(sock);

			return;
		}

		if (0 == _stricmp(method, "POST")) {
			for (int loop_i = 0; loop_i < content_length; loop_i++) {
				recv(sock, &c, 1, 0);
				cgi.Write((PBYTE)&c, 1);
			}
			c = '\n';
			cgi.Write((PBYTE)&c, 1);
		}

		cgi.Wait();
		char OutBuf[2048]{ 0 };
		cgi.Read((PBYTE)OutBuf, sizeof(OutBuf)-1);

		send(sock, OutBuf, strlen(OutBuf), 0);
		return;
	}



	// -----------------------------------------------------------------------*/
	//函数方法：not_found
	//作用：处理文件不存在的情况
	// -----------------------------------------------------------------------*/
	void not_found(SOCKET sock) {
		char *pResponse = "HTTP/1.0 404 NOT FOUND"\
			SERVER_STRING\
			"Content-Type:text/html\r\n"\
			"\r\n"\
			"<HTML><HEAD><TITLE> Not Found\r\n"\
			"<TITLE><HEAD>\r\n"\
			"<BODY><P> The server could not fufill\r\n"\
			"your request because the resource specified\r\n"\
			"is unavailable or nonexistent\r\n"\
			"<BODY><HTML>\r\n";

		send(sock, pResponse, strlen(pResponse), 0);
	}




	// -----------------------------------------------------------------------*/
	//函数方法：unimplemented
	//作用：将未实现method方法的请求返回给浏览器
	// -----------------------------------------------------------------------*/
	void unimplemented(SOCKET sock) {
		char *pResponse = "HTTP/1.0 501 METHOD NOT IMPLEMENTED"\
			SERVER_STRING\
			"Content-Type:text/html\r\n"\
			"\r\n"\
			"<HTML><HEAD><TITLE> Method Not Implemented\r\n"\
			"<TITLE><HEAD>\r\n"\
			"<BODY><P> HTTP request not supported.\r\n"\
			"<BODY><HTML>\r\n";
	
		send(sock, pResponse, strlen(pResponse), 0);
	}



	// -----------------------------------------------------------------------*/
	//函数方法：discardheaders
	//作用：读取并丢弃头数据
	// -----------------------------------------------------------------------*/
	void discardheaders(SOCKET sock) {
		char buf[1024]{ 0 };
		int numchars = 1;
		while (numchars > 0 && strcmp("\n", buf)) {
			numchars = get_line(sock, buf, sizeof(buf));
		}
	
	}



	// -----------------------------------------------------------------------*/
	//函数方法：error_die
	//作用：输出错误信息并退出
	// -----------------------------------------------------------------------*/
	void error_die(const char* cc)
	{
		perror(cc);
		exit(1);
	}



	// -----------------------------------------------------------------------*/
	//函数方法：headers
	//作用：返回给浏览器http响应头
	// -----------------------------------------------------------------------*/
	void headers(SOCKET sock) {
		char *pHeader = "HTTP/1.0 200 OK\r\n"\
			SERVER_STRING\
			"Content-Type:text/html\r\n"\
			"\r\n";

		send(sock, pHeader, strlen(pHeader), 0);
	}


	
	// -----------------------------------------------------------------------*/
	//源码创建管道处用到  这里没有用到 只列出
	//函数方法：cannot_execute
	//作用：处理执行CGI脚本时出错
	// -----------------------------------------------------------------------*/
	//
	//void cannot_execute(SOCKET sock)
	//
	// -----------------------------------------------------------------------*/





	// -----------------------------------------------------------------------*/
	//函数方法：bad_request
	//作用：响应客户端这是个错误请求
	// -----------------------------------------------------------------------*/
	void bad_request(SOCKET sock) {
		char *pResponse = "HTTP/1.0 400 BAD REQUEST\r\n"\
			"Content-Type:text/html\r\n"\
			"\r\n"\
			"<P>Your browser ent a bad request, such as a POST without a Content-Length.<P>\r\n";
	
		send(sock, pResponse, strlen(pResponse), 0);
	}



	// -----------------------------------------------------------------------*/
	//函数方法：cat
	//作用：把文件发送给客户端
	// -----------------------------------------------------------------------*/
	void cat(SOCKET sock, FILE *sourcefile) {
		char buf[1024]{ 0 };

		do {
			fgets(buf, sizeof(buf), sourcefile);
			size_t buflen = strlen(buf);
			if (buflen > 0) {
				send(sock, buf, buflen, 0);
			}
		} while (!feof(sourcefile));
	}





	// -----------------------------------------------------------------------*/
	//函数方法：startup
	//作用：初始化http服务
	// -----------------------------------------------------------------------*/
	SOCKET startup(u_short *port)
	{
		SOCKET httpd = INVALID_SOCKET;
		struct sockaddr_in sockname = { 0 };

		httpd = socket(AF_INET,SOCK_STREAM,0);
		if (httpd == INVALID_SOCKET) {
			error_die("startup socket");
		}
		sockname.sin_family = AF_INET;
		sockname.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		sockname.sin_port = htons(*port);

		if (bind(httpd, (const sockaddr*)&sockname, sizeof(sockname)) < 0) {
			error_die("startup bind");
		}
		
		if (*port == 0) {//动态分配端口
			int socknamelen = sizeof(sockname);
			if (-1 == getsockname(httpd, (sockaddr*)&sockname, &socknamelen)) {
				error_die("startup getsockname");
			}
		}

		if (listen(httpd, 5)) {
			error_die("startup listen");
		}

		return httpd;
	}
	private:
		friend class MasticTinyHttpdTask;
};

class MasticTinyHttpdTask :public Itask
{
public:
	MasticTinyHttpdTask(MasticTinyHttpd &mth,MasticTinyHttpd::SOCKET_CONTEXT &socket_context)
		:m_socket_context(socket_context)
		, m_mth(&mth){}
	virtual ~MasticTinyHttpdTask() {}

	virtual void RunItask() {
		m_mth->accept_request(m_socket_context);
	}
private:
	MasticTinyHttpd::SOCKET_CONTEXT m_socket_context;
	MasticTinyHttpd *m_mth;
};

int _tmain() {
	
	struct sockaddr_in client_name {0};
	int client_namelen = sizeof(client_name);

	//初始化socket
	u_short port = 80;
	WSADATA wsaData{ 0 };
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	MasticTinyHttpd TinyHttpServer;
	SOCKET sock_serve = INVALID_SOCKET;
	sock_serve = TinyHttpServer.startup(&port);
	printf("Mastic-TinyHttpd running on port:%u\n", port);

	//初始化线程池
	ThreadPool tp;
	tp.CreateThreadPool(5);

	while (1) {
		MasticTinyHttpd::SOCKET_CONTEXT socket_context_cintext;
		socket_context_cintext.socket_client = accept(sock_serve, (sockaddr *)&client_name, &client_namelen);
		if (INVALID_SOCKET == socket_context_cintext.socket_client) {
			TinyHttpServer.error_die("accept");
		}

		printf("Thread ID [%u] is accept new client connect:%u\n", ::GetCurrentThreadId(), socket_context_cintext.socket_client);
		Itask *pItask = new MasticTinyHttpdTask(TinyHttpServer,socket_context_cintext);
		tp.PushItask(pItask);
	}

	return 0;
}