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
	// ͨ��ʹ��select������peer�ͻ��˵���������
	int psConnection();
	//�����߳�
	static DWORD __stdcall workerThread(LPVOID pParam);
	
public:
	
	struct sockaddr_in saClient;
	int length;
	SOCKADDR_IN sSercice;
	//�߳̾��
	HANDLE Hthread;
	//socket�б�
	SOCKET g_SocketArr[FD_SETSIZE];
	int     g_iTotalConn = 0;
	fd_set         fdread;
};

