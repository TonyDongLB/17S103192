#include "stdafx.h"
#include "FileManage.h"


FileManage::FileManage()
{
	fileLength = 0;
	memset(fileName, '/0', MAX_FILE_LEN);
}


FileManage::~FileManage()
{
}


// 读取文件
int FileManage::readFile(char* path)
{
	//读取文件名
	this->path = path;
	string fPath(path);
	int pos = fPath.find_last_of('/');
	string s(fPath.substr(pos + 1));
	char * ps = (char*)s.c_str();
	int i = 0;
	while (ps[i]!='\0')
	{
		fileName[i] = ps[i++];
	}
	fileName[i] = '\0';
	//获取文件长度
	pFile = fopen(path, "rb");
	if (pFile == NULL)
	    perror("Error opening file");
	else
	{
		///将文件指针移动文件结尾
		fseek(pFile, 0, SEEK_END);  
		///求出当前文件指针距离文件开始的字节数
		fileLength = ftell(pFile); 
		fclose(pFile);
	}

	return 0;
}


// 设定socket
int FileManage::setSocket(SOCKET &skt)
{
	socket = skt;
	return 0;
}


// 发送文件
int FileManage::sendFile()
{
	int ret=0;
	int len = strlen(path);
	TCHAR *spath = new TCHAR[len + 1];
	mbstowcs(spath, path, len + 1);
	//先发送文件名
	ret = send(socket, fileName, strlen(fileName)+1, 0);
	if (ret == SOCKET_ERROR)
	{
		printf("文件名发送失败\n");
		ret = -1;
	}
	else
	{
		printf("文件名发送成功，继续发送文件数据\n");
		HANDLE hFile = CreateFile(spath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			//perror("文件打开错误");
			cout << GetLastError();
			return -1;
		}
		ret = TransmitFile(socket, hFile, 0, TRANS_FILE_LENGTH, NULL, NULL, TF_DISCONNECT | TF_REUSE_SOCKET);
		if (ret==FALSE)
		{
			cout << "文件发送错误，错误码：" << WSAGetLastError() << endl;
			ret = -1;
		}
		else
		{
			cout << fileName << "文件发送成功" << endl;
			ret = 0;
		}
		CloseHandle(hFile);
	}
	free(spath);
	spath = NULL;
	return ret;
}


// 接收文件
int FileManage::recvFile(char * SavefilePath)
{
	ULONG lReadSize = 0;
	char szInfo[TRANS_FILE_LENGTH] = { 0 };
	int len = strlen(SavefilePath);
	TCHAR *spath = new TCHAR[len + 1];
	mbstowcs(spath, SavefilePath, len + 1);
	HANDLE hFile = CreateFile(spath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		//perror("文件创建错误");
		cout <<"文件创建错误，错误码："<< GetLastError();
		return -1;
	}
	//接收文件名
	int ret = recv(socket, szInfo, TRANS_FILE_LENGTH, 0);
	//szInfo[ret] = '/0';
	int i = 0;
	while (szInfo[i] != '\0')
	{
		fileName[i] = szInfo[i++];
	}
	fileName[i] = '\0';
	while (true)
	{
		memset(szInfo, 0, sizeof(szInfo));
		int iSize = recv(socket, szInfo, TRANS_FILE_LENGTH, 0);
		if (iSize == SOCKET_ERROR || iSize == 0)
		{
			cout << "接受错误，错误码：" << WSAGetLastError() << endl;
			CloseHandle(hFile);
			break;
		}
		if (iSize < TRANS_FILE_LENGTH)
		{
			WriteFile(hFile, szInfo, iSize, &lReadSize, NULL);
			CloseHandle(hFile);
			break;
		}
		//写入文件 
		WriteFile(hFile, szInfo, iSize, &lReadSize, NULL);
	}
	cout << "文件" << fileName << "接收完毕" << endl;
	cout << "保存目录:" << SavefilePath << endl;
	free(spath);
	spath = NULL;
	return 0;
}



// int main()
// {
// 	FileManage fm;
// 	fm.readFile("D:/Efficient joint.pdf");
// 	return 0;
// }




// 计算文件的hash
string FileManage::getFileHash(char* filePath)
{
	int   i,j;
	HashSH1 filehash;
	FILE*   file;
	SHA1_CTX   context;
	unsigned   char   digest[20], buffer[16384];
	string strHash;
	if (!(file = fopen(filePath, "rb ")))   {

		cout << "无法打开文件" << endl;

		return "";
	}
	filehash.SHA1Init(&context);

	while (!feof(file))   {     /*   note:   what   if   ferror(file)   */

		i = fread(buffer, 1, 16384, file);

		filehash.SHA1Update(&context, buffer, i);

	}

	filehash.SHA1Final(digest, &context);

	fclose(file);
	char buf[3] = { 0 };
	
	for (i = 0; i < 5; i++)   {

		for (j = 0; j < 4; j++)   {
			sprintf((char*)buf, "%02X", digest[i * 4 + j]);
			strHash += string((char*)buf);
		}
	}

	return strHash;

}
