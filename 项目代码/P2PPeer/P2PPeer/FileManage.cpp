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


// ��ȡ�ļ�
int FileManage::readFile(char* path)
{
	//��ȡ�ļ���
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
	//��ȡ�ļ�����
	pFile = fopen(path, "rb");
	if (pFile == NULL)
	    perror("Error opening file");
	else
	{
		///���ļ�ָ���ƶ��ļ���β
		fseek(pFile, 0, SEEK_END);  
		///�����ǰ�ļ�ָ������ļ���ʼ���ֽ���
		fileLength = ftell(pFile); 
		fclose(pFile);
	}

	return 0;
}


// �趨socket
int FileManage::setSocket(SOCKET &skt)
{
	socket = skt;
	return 0;
}


// �����ļ�
int FileManage::sendFile()
{
	int ret=0;
	int len = strlen(path);
	TCHAR *spath = new TCHAR[len + 1];
	mbstowcs(spath, path, len + 1);
	//�ȷ����ļ���
	ret = send(socket, fileName, strlen(fileName)+1, 0);
	if (ret == SOCKET_ERROR)
	{
		printf("�ļ�������ʧ��\n");
		ret = -1;
	}
	else
	{
		printf("�ļ������ͳɹ������������ļ�����\n");
		HANDLE hFile = CreateFile(spath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			//perror("�ļ��򿪴���");
			cout << GetLastError();
			return -1;
		}
		ret = TransmitFile(socket, hFile, 0, TRANS_FILE_LENGTH, NULL, NULL, TF_DISCONNECT | TF_REUSE_SOCKET);
		if (ret==FALSE)
		{
			cout << "�ļ����ʹ��󣬴����룺" << WSAGetLastError() << endl;
			ret = -1;
		}
		else
		{
			cout << fileName << "�ļ����ͳɹ�" << endl;
			ret = 0;
		}
		CloseHandle(hFile);
	}
	free(spath);
	spath = NULL;
	return ret;
}


// �����ļ�
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
		//perror("�ļ���������");
		cout <<"�ļ��������󣬴����룺"<< GetLastError();
		return -1;
	}
	//�����ļ���
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
			cout << "���ܴ��󣬴����룺" << WSAGetLastError() << endl;
			CloseHandle(hFile);
			break;
		}
		if (iSize < TRANS_FILE_LENGTH)
		{
			WriteFile(hFile, szInfo, iSize, &lReadSize, NULL);
			CloseHandle(hFile);
			break;
		}
		//д���ļ� 
		WriteFile(hFile, szInfo, iSize, &lReadSize, NULL);
	}
	cout << "�ļ�" << fileName << "�������" << endl;
	cout << "����Ŀ¼:" << SavefilePath << endl;
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




// �����ļ���hash
string FileManage::getFileHash(char* filePath)
{
	int   i,j;
	HashSH1 filehash;
	FILE*   file;
	SHA1_CTX   context;
	unsigned   char   digest[20], buffer[16384];
	string strHash;
	if (!(file = fopen(filePath, "rb ")))   {

		cout << "�޷����ļ�" << endl;

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
