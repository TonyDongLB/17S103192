#pragma once
//#include <WinSock2.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "FileManage.h"

#pragma comment(lib, "ws2_32.lib")
//#define SEND_PORT 8001
#define SERVER_PORT 7777
#define PEER_SERVER_PORT 7701
//#define RECEIVE_PORT 8000
#define MAX_BUF_LEN 1024
#define CENTER_SERVICE_IP "127.0.0.1"
using namespace std;
class PeerClient
{
public:
	PeerClient();
	~PeerClient();
private:

	//��ѯ��������ַ��Ϣ
	struct sockaddr_in saServer;
	//peer��������ַ��Ϣ
	struct sockaddr_in peerServer;
	SOCKET connectSocketTcp;
	FileManage fileManage;
public:
	// �����Ҫ������ļ�
	int addFile(char* filePath);
	// ɾ����ǰ�ͻ��˹�����ļ�
	int deleteFile(char* filePath);
	// չʾ����˿ɹ����ص������ļ���Ϣ
	int showFileList();
	// �����Ự
	int closeSession();
	// ��ȡָ���ļ�
	int getFile(char* fName);
	// ���ӵ����ķ�����
	int connectToCServ();
	// �˳���¼
	int quitFromSer();
};

