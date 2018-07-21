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

	//查询服务器地址信息
	struct sockaddr_in saServer;
	//peer服务器地址信息
	struct sockaddr_in peerServer;
	SOCKET connectSocketTcp;
	FileManage fileManage;
public:
	// 添加需要共享的文件
	int addFile(char* filePath);
	// 删除当前客户端共享的文件
	int deleteFile(char* filePath);
	// 展示服务端可供下载的所有文件信息
	int showFileList();
	// 结束会话
	int closeSession();
	// 获取指定文件
	int getFile(char* fName);
	// 连接到中心服务器
	int connectToCServ();
	// 退出登录
	int quitFromSer();
};

