#include "stdafx.h"
#include "PeerClient.h"


PeerClient::PeerClient()
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
}


PeerClient::~PeerClient()
{
}

// 连接到中心服务器
int PeerClient::connectToCServ()
{
	// 创建 TCP 套接字
	int err;
	connectSocketTcp = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == connectSocketTcp) {
		err = WSAGetLastError();
		printf_s("\"socket\" error! error code is %d\n", err);
		return -1;
	}
	//构建服务器地址信息  
	saServer.sin_family = AF_INET;
	saServer.sin_port = htons(SERVER_PORT); //注意转化为网络节序  
	saServer.sin_addr.S_un.S_addr = inet_addr(CENTER_SERVICE_IP);
	peerServer.sin_family = AF_INET;
	peerServer.sin_port = htons(PEER_SERVER_PORT);
	// 发送缓存
	char sendBuff[MAX_BUF_LEN] = {0};
	// 接受缓存
	char receiveBuff[MAX_BUF_LEN] = {0};
	int nAddrLen = sizeof(SOCKADDR);
	err = connect(connectSocketTcp, (struct sockaddr *)&saServer, sizeof(saServer));
	if (err == SOCKET_ERROR)
	{
		printf_s("connect() 失败!\n");
		closesocket(connectSocketTcp); //关闭套接字  
		WSACleanup();
		return -1;
	}
	//发送注册信息"CONNECT"
	sprintf_s(sendBuff, 8, "%s", "CONNECT");
	// 发送数据通过 TCP
		int nSendSize = send(connectSocketTcp, sendBuff, strlen(sendBuff)+1, 0);
		if (SOCKET_ERROR == nSendSize) {
			err = WSAGetLastError();
			printf_s("\"sendto\" 错误!, 错误码 %d\n", err);
			return -1;
		}
		printf_s("已发送: %s\n", sendBuff);
		//接收数据通过TCP
		int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
		if (SOCKET_ERROR == nReceiveSize) {
			err = WSAGetLastError();
			printf_s("\"recvfrom\" 错误!, 错误码 %d\n", err);
			return -1;
		}
		//receiveBuff[nReceiveSize] = '\0';
		printf_s("Receive: %s\n", receiveBuff);
		if (!strncmp("ACCEPT",receiveBuff,6))
		{
			return 0;
		} 
		else
		{
			return -1;
		}
}

// 添加需要共享的文件
int PeerClient::addFile(char* filePath)
{
	string fileName(filePath);
	string hashValue;
	int fileSize;
	// 发送缓存
	//char sendBuff[MAX_BUF_LEN] = { 0 };
	// 接受缓存
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	//获取文件名
	//int pos = fileName.find_last_of('/');
	//fileName = fileName.substr(pos + 1);
	//获取文件大小
	FILE* pFile = fopen(filePath, "rb");
	if (pFile == NULL)
	{
		perror("文件打开错误");
		return - 1;

	}
	else
	{
		///将文件指针移动文件结尾
		fseek(pFile, 0, SEEK_END);
		///求出当前文件指针距离文件开始的字节数
		fileSize = ftell(pFile);
		fclose(pFile);
	}
	//计算文件的hash值
	hashValue = fileManage.getFileHash(filePath);
	//待发送的字符串
	string strToSend = "ADD " + fileName +" "+ hashValue +" "+ string(to_string(fileSize));
	// 发送数据通过 TCP
	int nSendSize = send(connectSocketTcp, strToSend.data(), strToSend.length()+1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" 错误!, 错误码 %d\n", err);
		return -1;
	}
	//printf_s("已发送: %s\n", strToSend);
	cout << "已发送信息" << strToSend;
	//接收数据通过TCP
	int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
	if (SOCKET_ERROR == nReceiveSize) {
		err = WSAGetLastError();
		printf_s("\"recvfrom\" 错误!, 错误码 %d\n", err);
		return -1;
	}
	//receiveBuff[nReceiveSize] = '\0';
	//printf_s("Receive: %s\n", receiveBuff);
	if (!strncmp("OK", receiveBuff,2))
	{
		cout << "添加成功！" << endl;
		return 0;
	}
	else
	{
		cout << "添加失败！" << receiveBuff<<endl;
		return -1;
	}
}


// 删除当前客户端共享的文件
int PeerClient::deleteFile(char* filePath)
{
	string fileName(filePath);
	string hashValue;
	//int fileSize;
	// 接受缓存
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	//获取文件名
	//int pos = fileName.find_last_of('/');
	//fileName = fileName.substr(pos + 1);
	//获取文件大小
	/*FILE* pFile = fopen(filePath, "rb");
	if (pFile == NULL)
		perror("文件打开错误");
	else
	{
		///将文件指针移动文件结尾
		fseek(pFile, 0, SEEK_END);
		///求出当前文件指针距离文件开始的字节数
		fileSize = ftell(pFile);
		fclose(pFile);
	}*/
	//计算文件的hash值
	hashValue = fileManage.getFileHash(filePath);
	//待发送的字符串
	string strToSend = "DELETE " + fileName +" "+ hashValue;
	// 发送数据通过 TCP
	int nSendSize = send(connectSocketTcp, strToSend.data(), strToSend.length()+1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" 错误!, 错误码 %d\n", err);
		return -1;
	}
	//printf_s("已发送: %s\n", strToSend);
	cout << "已发送：" << strToSend << endl;
	//接收数据通过TCP
	int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
	if (SOCKET_ERROR == nReceiveSize) {
		err = WSAGetLastError();
		printf_s("\"recvfrom\" 错误!, 错误码 %d\n", err);
		return -1;
	}
	//receiveBuff[nReceiveSize] = '\0';
	//printf_s("Receive: %s\n", receiveBuff);
	if (!strncmp("OK", receiveBuff, 2))
	{
		cout << "删除成功！" << endl;
		return 0;
	}
	else
	{
		cout << "删除失败！" << receiveBuff << endl;
		return -1;
	}
}


// 展示服务端可供下载的所有文件信息
int PeerClient::showFileList()
{
	/*string fileName(filePath);
	string hashValue;
	//int fileSize;*/
	// 接受缓存
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	/*//获取文件名
	int pos = fileName.find_last_of('/');
	fileName = fileName.substr(pos + 1);
	//获取文件大小
	/*FILE* pFile = fopen(filePath, "rb");
	if (pFile == NULL)
	perror("文件打开错误");
	else
	{
	///将文件指针移动文件结尾
	fseek(pFile, 0, SEEK_END);
	///求出当前文件指针距离文件开始的字节数
	fileSize = ftell(pFile);
	fclose(pFile);
	}
	//计算文件的hash值
	hashValue = fileManage.getFileHash(filePath);*/
	//待发送的字符串
	string strToSend = "LIST";
	// 发送数据通过 TCP
	int nSendSize = send(connectSocketTcp, strToSend.data(), strToSend.length()+1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" 错误!, 错误码 %d\n", err);
		return -1;
	}
	//printf_s("已发送: %s\n", strToSend);
	cout << "已发送" << strToSend << endl;
	//接收数据通过TCP
	int nReceiveSize;
	while (true)
	{
		memset(receiveBuff, 0, sizeof(receiveBuff));
		nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
		if (SOCKET_ERROR == nReceiveSize) {
			err = WSAGetLastError();
			printf_s("\"recvfrom\" 错误!, 错误码 %d\n", err);
			return -1;
		}
		//receiveBuff[nReceiveSize] = '\0';
		//printf_s("Receive: %s\n", receiveBuff);
		if (!strncmp("OK", receiveBuff, 2))
		{
			cout << "查询结束" << endl;
			return 0;
		}
		string msg(receiveBuff);
		if (msg.find("ERROR") == string::npos)
		{
			//strtok(receiveBuff, " ");
			char* fName = strtok(receiveBuff, " ");
			char *fSize;
			if (fName != NULL)
			{
				fSize = strtok(NULL, " ");
				if (fSize!=NULL)
				{
					//获取文件名
					string fileName(fName);
					int pos = fileName.find_last_of('/');
					fileName = fileName.substr(pos + 1);
					cout << "文件名：" << fileName << " 文件大小" << fSize << endl;
				} 
				else
				{
					cout << "获取文件大小失败" << endl;
					return -1;
				}
			}
			else
			{
				cout << "获取文件名失败" << endl;
				return -1;
			}
			
		} 
		else
		{
			cout << "获取失败！" << receiveBuff << endl;
			return -1;
		}
	}
}


// 结束会话
int PeerClient::closeSession()
{
	cout << "会话结束" << endl;
	closesocket(connectSocketTcp);
	return 0;
}


// 获取指定文件
int PeerClient::getFile(char* fName)
{
	// 接受缓存
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	//待发送的字符串
	string strToSend = "REQUEST "  + string(fName);
	// 发送数据通过 TCP
	int nSendSize = send(connectSocketTcp, strToSend.data(), strToSend.length()+1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" 错误!, 错误码 %d\n", err);
		return -1;
	}
	//printf_s("已发送: %s\n", strToSend);
	cout << "已发送: " << strToSend << endl;
	//接收数据通过TCP
	while (true)
	{
		memset(receiveBuff, 0, sizeof(receiveBuff));
		int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
		if (SOCKET_ERROR == nReceiveSize) {
			err = WSAGetLastError();
			printf_s("\"recvfrom\" 错误!, 错误码 %d\n", err);
			return -1;
		}
		string msg(receiveBuff);
		if (msg.find("ERROR") != string::npos)
		{
			cout << msg;
			return -1;
		}
		else if (!strncmp("OK", receiveBuff, 2))
		{
			cout << "获取成功！" << endl;
			return 0;
		}
		else
		{
			//strtok(receiveBuff, " ");
			char* filepath = strtok(receiveBuff, " ");
			char* peerIP = strtok(NULL, " ");
			char *fSize = strtok(NULL, " ");
			cout << "文件名:" << fName << "目标IP:" << peerIP << "文件大小：" << fSize << endl;
			//向指定ip的peer请求文件
			//文件保存路径
			char fileToSave[MAX_FILE_LEN];
			cout << "请输入文件保存目录路径：\n";
			cin >> fileToSave;
			char *sep = "/";
			strcat(fileToSave, sep);
			strcat(fileToSave, fName);
			// 创建 TCP 套接字
			SOCKET conntSocketTcp = socket(AF_INET, SOCK_STREAM, 0);
			if (INVALID_SOCKET == conntSocketTcp) {
				err = WSAGetLastError();
				printf_s("\"socket\" error! error code is %d\n", err);
				return -1;
			}
			//构建服务器地址信息  
			peerServer.sin_addr.S_un.S_addr = inet_addr(peerIP);
			//连接服务器  
			err = connect(conntSocketTcp, (struct sockaddr *)&peerServer, sizeof(peerServer));
			if (err == SOCKET_ERROR)
			{
				printf_s("connect() failed!\n");
				closesocket(conntSocketTcp); //关闭套接字  
				WSACleanup();
				return -1;
			}
			// 发送文件路径通过 TCP
			int nSendSize = send(conntSocketTcp, filepath, strlen(filepath) + 1, 0);
			Sleep(10);
			if (SOCKET_ERROR == nSendSize) {
				err = WSAGetLastError();
				printf_s("\"sendto\" 错误!, 错误码 %d\n", err);
				return -1;
			}
			fileManage.setSocket(conntSocketTcp);
			fileManage.recvFile(fileToSave);
		}
	}
	return 0;
}


// 退出登录
int PeerClient::quitFromSer()
{
	// 接受缓存
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	//待发送的字符串
	char* strToSend = "QUIT";
	// 发送数据通过 TCP
	int nSendSize = send(connectSocketTcp, strToSend, strlen(strToSend) + 1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" 错误!, 错误码 %d\n", err);
		return -1;
	}
	int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
	if (SOCKET_ERROR == nReceiveSize) {
		err = WSAGetLastError();
		printf_s("\"recvfrom\" 错误!, 错误码 %d\n", err);
		return -1;
	}
	if (!strncmp("GOODBYE", receiveBuff,7))
	{
		cout << "会话结束" << endl;
		closesocket(connectSocketTcp);
		return 0;
	}
	else
	{
		cout << "退出登录错误" << endl;
		return -1;
	}
}
