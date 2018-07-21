#include "stdafx.h"
#include "PeerServer.h"

//DWORD WINAPI WorkerThread(LPVOID lpParameter);
PeerServer::PeerServer()
{
	// 加载套接字库（必须）
	WORD wVersionRequested;
	WSADATA wsaData;
	// 套接字加载时错误提示
	int err;
	// 启动 socket api,版本 2.2
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		//不到 winsock.dll
		printf_s("WSAStartup failed with error: %d\n", err);
		return;
	}
	// 低字节，高字节
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		// 版本错误
		printf_s("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
		return;
	}
	length = 0;
	SOCKET skt = socket(AF_INET, SOCK_STREAM, 0);
	sSercice.sin_family = AF_INET;
	sSercice.sin_port = htons(DEFAULT_MSG_PORT);
	sSercice.sin_addr.s_addr = 0;
	err = bind(skt, (SOCKADDR*)&sSercice, sizeof(SOCKADDR));
	if (SOCKET_ERROR == err) {
		err = WSAGetLastError();
		printf("\"bind\" error! error code is %d\n", err);
	}
	else
	{
		//加入到套接字列表
		g_SocketArr[g_iTotalConn++] = skt;
	}
	//监听连接请求
	err = listen(skt, 5);
	if (err == SOCKET_ERROR)
	{
		printf("listen() faild! code:%d\n", WSAGetLastError());
		closesocket(skt); //关闭套接字  
	}
}

// 通过使用select来处理peer客户端的连接请求
int PeerServer::psConnection()
{
	DWORD       dwThreadId;
	// Create worker thread   
	Hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)workerThread, (LPVOID)this, 0, &dwThreadId);
	return 0;
}
DWORD __stdcall PeerServer::workerThread(LPVOID pParam)
{
	PeerServer *pServer = (PeerServer*)pParam;
	int ret = 0;
	int i;
	char szMessage[MSGSIZE];
	while (true)
	{
		FD_ZERO(&(pServer->fdread));
		for (i = 0; i < pServer->g_iTotalConn; i++)
		{
			FD_SET((pServer->g_SocketArr[i]), &(pServer->fdread));
		}
		ret = select(0, &(pServer->fdread), NULL, NULL, 0);
		for (i = 0; i < pServer->g_iTotalConn; i++)
		{
			if (FD_ISSET((pServer->g_SocketArr[i]), &(pServer->fdread)))
			{
				SOCKET* handleSocket = &(pServer->g_SocketArr[i]);
				//处理TCP连接
				if (i == 0)
				{
					pServer->length = sizeof(pServer->saClient);
					SOCKET sClient = accept((pServer->g_SocketArr[i]), (struct sockaddr *)&(pServer->saClient), &(pServer->length));
					printf("接受客户端:%s:%d\n", inet_ntoa((pServer->saClient).sin_addr), ntohs((pServer->saClient).sin_port));
					//char IPdotdec[20];
					//inet_ntop(AF_INET, &saClient.sin_addr, IPdotdec, 16);
					//printf("接受客户端:%s:%d\n", IPdotdec, ntohs(saClient.sin_port));
					if (sClient != INVALID_SOCKET)
					{
						//加入套接字列表     
						pServer->g_SocketArr[pServer->g_iTotalConn++] = sClient;
					}
					handleSocket = &sClient;
				}
				ret = recv(*handleSocket, szMessage, MSGSIZE, 0);
				if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET))
				{
					//关闭这个连接        
					printf("客户端 socket %d 已关闭.\n", *handleSocket);
					closesocket(*handleSocket);
					if (i < pServer->g_iTotalConn - 1)
					{
						pServer->g_SocketArr[i--] = pServer->g_SocketArr[--(pServer->g_iTotalConn)];
					}
					else
					{
						--i;
						--(pServer->g_iTotalConn);
					}
				}
				else
				{
					// 处理客户端请求的文件,此消息应为文件完整路径,并向客户端发送文件
					//szMessage[ret] = '\0';
					FileManage fileManage;
					fileManage.setSocket(*handleSocket);
					fileManage.readFile(szMessage);
					fileManage.sendFile();

				}

			}
		}

	}
	return ret;
}

PeerServer::~PeerServer()
{
	for (int i = 0; i < g_iTotalConn; i++)
	{
		closesocket(g_SocketArr[i]);
	}
	WSACleanup();
}
