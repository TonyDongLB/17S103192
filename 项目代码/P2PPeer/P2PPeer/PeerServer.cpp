#include "stdafx.h"
#include "PeerServer.h"

//DWORD WINAPI WorkerThread(LPVOID lpParameter);
PeerServer::PeerServer()
{
	// �����׽��ֿ⣨���룩
	WORD wVersionRequested;
	WSADATA wsaData;
	// �׽��ּ���ʱ������ʾ
	int err;
	// ���� socket api,�汾 2.2
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		//���� winsock.dll
		printf_s("WSAStartup failed with error: %d\n", err);
		return;
	}
	// ���ֽڣ����ֽ�
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		// �汾����
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
		//���뵽�׽����б�
		g_SocketArr[g_iTotalConn++] = skt;
	}
	//������������
	err = listen(skt, 5);
	if (err == SOCKET_ERROR)
	{
		printf("listen() faild! code:%d\n", WSAGetLastError());
		closesocket(skt); //�ر��׽���  
	}
}

// ͨ��ʹ��select������peer�ͻ��˵���������
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
				//����TCP����
				if (i == 0)
				{
					pServer->length = sizeof(pServer->saClient);
					SOCKET sClient = accept((pServer->g_SocketArr[i]), (struct sockaddr *)&(pServer->saClient), &(pServer->length));
					printf("���ܿͻ���:%s:%d\n", inet_ntoa((pServer->saClient).sin_addr), ntohs((pServer->saClient).sin_port));
					//char IPdotdec[20];
					//inet_ntop(AF_INET, &saClient.sin_addr, IPdotdec, 16);
					//printf("���ܿͻ���:%s:%d\n", IPdotdec, ntohs(saClient.sin_port));
					if (sClient != INVALID_SOCKET)
					{
						//�����׽����б�     
						pServer->g_SocketArr[pServer->g_iTotalConn++] = sClient;
					}
					handleSocket = &sClient;
				}
				ret = recv(*handleSocket, szMessage, MSGSIZE, 0);
				if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET))
				{
					//�ر��������        
					printf("�ͻ��� socket %d �ѹر�.\n", *handleSocket);
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
					// ����ͻ���������ļ�,����ϢӦΪ�ļ�����·��,����ͻ��˷����ļ�
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
