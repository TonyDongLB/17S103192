#pragma once
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <string>
#include <iostream>
#include <Mswsock.h>
#include "HashSH1.h"
//每次发送的数据块大小
#define TRANS_FILE_LENGTH 1024
//最大文件名长度
#define MAX_FILE_LEN 1024
//最大文件缓存
#define SIZE_OF_BUFFER 16000
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "ws2_32.lib")
using namespace std;
class FileManage
{
public:
	FileManage();
	~FileManage();
private:
	char fileName[MAX_FILE_LEN];
	int fileLength;
	char *fileBuffer;
	SOCKET socket;
	//文件路径
	char *path;
	//文件
	FILE * pFile;
	//待保存的文件路径
	//char *pathToSave;
	//待保存文件
	//FILE * pFiletoSave;
public:
	// 读取文件
	int readFile(char* path);
	//设定socket
	int setSocket(SOCKET &skt);
	// 发送文件
	int sendFile();
	// 接收文件
	int recvFile(char * SavefilePath);
	// 保存文件
	int saveFile(char* path);
	// 计算文件的hash
	string getFileHash(char* filePath);
};

