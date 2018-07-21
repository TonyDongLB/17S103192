#pragma once
#include "Thread.h"
#include <iostream>
//#include <WinSock2.h>
#include "FileManage.h"
#define DEFAULT_MSG_PORT 7701
#define DEFAULT_FILE_PORT 7702
#define MAX_BUF_LEN 1024
#define MSGSIZE     1024
#pragma comment(lib, "ws2_32.lib")
using namespace std;
class PeerServer /*:public Runnable*/
{
public:
	PeerServer();
	~PeerServer();
	// 通过使用select来处理peer客户端的连接请求
	int psConnection();
	//工作线程
	static DWORD __stdcall workerThread(LPVOID pParam);
	
public:
	
	struct sockaddr_in saClient;
	int length;
	SOCKADDR_IN sSercice;
	//线程句柄
	HANDLE Hthread;
	//socket列表
	SOCKET g_SocketArr[FD_SETSIZE];
	int     g_iTotalConn = 0;
	fd_set         fdread;
};

