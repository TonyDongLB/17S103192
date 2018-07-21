#pragma once
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <string>
#include <iostream>
#include <Mswsock.h>
#include "HashSH1.h"
//ÿ�η��͵����ݿ��С
#define TRANS_FILE_LENGTH 1024
//����ļ�������
#define MAX_FILE_LEN 1024
//����ļ�����
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
	//�ļ�·��
	char *path;
	//�ļ�
	FILE * pFile;
	//��������ļ�·��
	//char *pathToSave;
	//�������ļ�
	//FILE * pFiletoSave;
public:
	// ��ȡ�ļ�
	int readFile(char* path);
	//�趨socket
	int setSocket(SOCKET &skt);
	// �����ļ�
	int sendFile();
	// �����ļ�
	int recvFile(char * SavefilePath);
	// �����ļ�
	int saveFile(char* path);
	// �����ļ���hash
	string getFileHash(char* filePath);
};

