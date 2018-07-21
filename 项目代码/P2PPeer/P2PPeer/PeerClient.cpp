#include "stdafx.h"
#include "PeerClient.h"


PeerClient::PeerClient()
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
}


PeerClient::~PeerClient()
{
}

// ���ӵ����ķ�����
int PeerClient::connectToCServ()
{
	// ���� TCP �׽���
	int err;
	connectSocketTcp = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == connectSocketTcp) {
		err = WSAGetLastError();
		printf_s("\"socket\" error! error code is %d\n", err);
		return -1;
	}
	//������������ַ��Ϣ  
	saServer.sin_family = AF_INET;
	saServer.sin_port = htons(SERVER_PORT); //ע��ת��Ϊ�������  
	saServer.sin_addr.S_un.S_addr = inet_addr(CENTER_SERVICE_IP);
	peerServer.sin_family = AF_INET;
	peerServer.sin_port = htons(PEER_SERVER_PORT);
	// ���ͻ���
	char sendBuff[MAX_BUF_LEN] = {0};
	// ���ܻ���
	char receiveBuff[MAX_BUF_LEN] = {0};
	int nAddrLen = sizeof(SOCKADDR);
	err = connect(connectSocketTcp, (struct sockaddr *)&saServer, sizeof(saServer));
	if (err == SOCKET_ERROR)
	{
		printf_s("connect() ʧ��!\n");
		closesocket(connectSocketTcp); //�ر��׽���  
		WSACleanup();
		return -1;
	}
	//����ע����Ϣ"CONNECT"
	sprintf_s(sendBuff, 8, "%s", "CONNECT");
	// ��������ͨ�� TCP
		int nSendSize = send(connectSocketTcp, sendBuff, strlen(sendBuff)+1, 0);
		if (SOCKET_ERROR == nSendSize) {
			err = WSAGetLastError();
			printf_s("\"sendto\" ����!, ������ %d\n", err);
			return -1;
		}
		printf_s("�ѷ���: %s\n", sendBuff);
		//��������ͨ��TCP
		int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
		if (SOCKET_ERROR == nReceiveSize) {
			err = WSAGetLastError();
			printf_s("\"recvfrom\" ����!, ������ %d\n", err);
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

// �����Ҫ������ļ�
int PeerClient::addFile(char* filePath)
{
	string fileName(filePath);
	string hashValue;
	int fileSize;
	// ���ͻ���
	//char sendBuff[MAX_BUF_LEN] = { 0 };
	// ���ܻ���
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	//��ȡ�ļ���
	//int pos = fileName.find_last_of('/');
	//fileName = fileName.substr(pos + 1);
	//��ȡ�ļ���С
	FILE* pFile = fopen(filePath, "rb");
	if (pFile == NULL)
	{
		perror("�ļ��򿪴���");
		return - 1;

	}
	else
	{
		///���ļ�ָ���ƶ��ļ���β
		fseek(pFile, 0, SEEK_END);
		///�����ǰ�ļ�ָ������ļ���ʼ���ֽ���
		fileSize = ftell(pFile);
		fclose(pFile);
	}
	//�����ļ���hashֵ
	hashValue = fileManage.getFileHash(filePath);
	//�����͵��ַ���
	string strToSend = "ADD " + fileName +" "+ hashValue +" "+ string(to_string(fileSize));
	// ��������ͨ�� TCP
	int nSendSize = send(connectSocketTcp, strToSend.data(), strToSend.length()+1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" ����!, ������ %d\n", err);
		return -1;
	}
	//printf_s("�ѷ���: %s\n", strToSend);
	cout << "�ѷ�����Ϣ" << strToSend;
	//��������ͨ��TCP
	int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
	if (SOCKET_ERROR == nReceiveSize) {
		err = WSAGetLastError();
		printf_s("\"recvfrom\" ����!, ������ %d\n", err);
		return -1;
	}
	//receiveBuff[nReceiveSize] = '\0';
	//printf_s("Receive: %s\n", receiveBuff);
	if (!strncmp("OK", receiveBuff,2))
	{
		cout << "��ӳɹ���" << endl;
		return 0;
	}
	else
	{
		cout << "���ʧ�ܣ�" << receiveBuff<<endl;
		return -1;
	}
}


// ɾ����ǰ�ͻ��˹�����ļ�
int PeerClient::deleteFile(char* filePath)
{
	string fileName(filePath);
	string hashValue;
	//int fileSize;
	// ���ܻ���
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	//��ȡ�ļ���
	//int pos = fileName.find_last_of('/');
	//fileName = fileName.substr(pos + 1);
	//��ȡ�ļ���С
	/*FILE* pFile = fopen(filePath, "rb");
	if (pFile == NULL)
		perror("�ļ��򿪴���");
	else
	{
		///���ļ�ָ���ƶ��ļ���β
		fseek(pFile, 0, SEEK_END);
		///�����ǰ�ļ�ָ������ļ���ʼ���ֽ���
		fileSize = ftell(pFile);
		fclose(pFile);
	}*/
	//�����ļ���hashֵ
	hashValue = fileManage.getFileHash(filePath);
	//�����͵��ַ���
	string strToSend = "DELETE " + fileName +" "+ hashValue;
	// ��������ͨ�� TCP
	int nSendSize = send(connectSocketTcp, strToSend.data(), strToSend.length()+1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" ����!, ������ %d\n", err);
		return -1;
	}
	//printf_s("�ѷ���: %s\n", strToSend);
	cout << "�ѷ��ͣ�" << strToSend << endl;
	//��������ͨ��TCP
	int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
	if (SOCKET_ERROR == nReceiveSize) {
		err = WSAGetLastError();
		printf_s("\"recvfrom\" ����!, ������ %d\n", err);
		return -1;
	}
	//receiveBuff[nReceiveSize] = '\0';
	//printf_s("Receive: %s\n", receiveBuff);
	if (!strncmp("OK", receiveBuff, 2))
	{
		cout << "ɾ���ɹ���" << endl;
		return 0;
	}
	else
	{
		cout << "ɾ��ʧ�ܣ�" << receiveBuff << endl;
		return -1;
	}
}


// չʾ����˿ɹ����ص������ļ���Ϣ
int PeerClient::showFileList()
{
	/*string fileName(filePath);
	string hashValue;
	//int fileSize;*/
	// ���ܻ���
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	/*//��ȡ�ļ���
	int pos = fileName.find_last_of('/');
	fileName = fileName.substr(pos + 1);
	//��ȡ�ļ���С
	/*FILE* pFile = fopen(filePath, "rb");
	if (pFile == NULL)
	perror("�ļ��򿪴���");
	else
	{
	///���ļ�ָ���ƶ��ļ���β
	fseek(pFile, 0, SEEK_END);
	///�����ǰ�ļ�ָ������ļ���ʼ���ֽ���
	fileSize = ftell(pFile);
	fclose(pFile);
	}
	//�����ļ���hashֵ
	hashValue = fileManage.getFileHash(filePath);*/
	//�����͵��ַ���
	string strToSend = "LIST";
	// ��������ͨ�� TCP
	int nSendSize = send(connectSocketTcp, strToSend.data(), strToSend.length()+1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" ����!, ������ %d\n", err);
		return -1;
	}
	//printf_s("�ѷ���: %s\n", strToSend);
	cout << "�ѷ���" << strToSend << endl;
	//��������ͨ��TCP
	int nReceiveSize;
	while (true)
	{
		memset(receiveBuff, 0, sizeof(receiveBuff));
		nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
		if (SOCKET_ERROR == nReceiveSize) {
			err = WSAGetLastError();
			printf_s("\"recvfrom\" ����!, ������ %d\n", err);
			return -1;
		}
		//receiveBuff[nReceiveSize] = '\0';
		//printf_s("Receive: %s\n", receiveBuff);
		if (!strncmp("OK", receiveBuff, 2))
		{
			cout << "��ѯ����" << endl;
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
					//��ȡ�ļ���
					string fileName(fName);
					int pos = fileName.find_last_of('/');
					fileName = fileName.substr(pos + 1);
					cout << "�ļ�����" << fileName << " �ļ���С" << fSize << endl;
				} 
				else
				{
					cout << "��ȡ�ļ���Сʧ��" << endl;
					return -1;
				}
			}
			else
			{
				cout << "��ȡ�ļ���ʧ��" << endl;
				return -1;
			}
			
		} 
		else
		{
			cout << "��ȡʧ�ܣ�" << receiveBuff << endl;
			return -1;
		}
	}
}


// �����Ự
int PeerClient::closeSession()
{
	cout << "�Ự����" << endl;
	closesocket(connectSocketTcp);
	return 0;
}


// ��ȡָ���ļ�
int PeerClient::getFile(char* fName)
{
	// ���ܻ���
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	//�����͵��ַ���
	string strToSend = "REQUEST "  + string(fName);
	// ��������ͨ�� TCP
	int nSendSize = send(connectSocketTcp, strToSend.data(), strToSend.length()+1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" ����!, ������ %d\n", err);
		return -1;
	}
	//printf_s("�ѷ���: %s\n", strToSend);
	cout << "�ѷ���: " << strToSend << endl;
	//��������ͨ��TCP
	while (true)
	{
		memset(receiveBuff, 0, sizeof(receiveBuff));
		int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
		if (SOCKET_ERROR == nReceiveSize) {
			err = WSAGetLastError();
			printf_s("\"recvfrom\" ����!, ������ %d\n", err);
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
			cout << "��ȡ�ɹ���" << endl;
			return 0;
		}
		else
		{
			//strtok(receiveBuff, " ");
			char* filepath = strtok(receiveBuff, " ");
			char* peerIP = strtok(NULL, " ");
			char *fSize = strtok(NULL, " ");
			cout << "�ļ���:" << fName << "Ŀ��IP:" << peerIP << "�ļ���С��" << fSize << endl;
			//��ָ��ip��peer�����ļ�
			//�ļ�����·��
			char fileToSave[MAX_FILE_LEN];
			cout << "�������ļ�����Ŀ¼·����\n";
			cin >> fileToSave;
			char *sep = "/";
			strcat(fileToSave, sep);
			strcat(fileToSave, fName);
			// ���� TCP �׽���
			SOCKET conntSocketTcp = socket(AF_INET, SOCK_STREAM, 0);
			if (INVALID_SOCKET == conntSocketTcp) {
				err = WSAGetLastError();
				printf_s("\"socket\" error! error code is %d\n", err);
				return -1;
			}
			//������������ַ��Ϣ  
			peerServer.sin_addr.S_un.S_addr = inet_addr(peerIP);
			//���ӷ�����  
			err = connect(conntSocketTcp, (struct sockaddr *)&peerServer, sizeof(peerServer));
			if (err == SOCKET_ERROR)
			{
				printf_s("connect() failed!\n");
				closesocket(conntSocketTcp); //�ر��׽���  
				WSACleanup();
				return -1;
			}
			// �����ļ�·��ͨ�� TCP
			int nSendSize = send(conntSocketTcp, filepath, strlen(filepath) + 1, 0);
			Sleep(10);
			if (SOCKET_ERROR == nSendSize) {
				err = WSAGetLastError();
				printf_s("\"sendto\" ����!, ������ %d\n", err);
				return -1;
			}
			fileManage.setSocket(conntSocketTcp);
			fileManage.recvFile(fileToSave);
		}
	}
	return 0;
}


// �˳���¼
int PeerClient::quitFromSer()
{
	// ���ܻ���
	char receiveBuff[MAX_BUF_LEN] = { 0 };
	int err;
	//�����͵��ַ���
	char* strToSend = "QUIT";
	// ��������ͨ�� TCP
	int nSendSize = send(connectSocketTcp, strToSend, strlen(strToSend) + 1, 0);
	if (SOCKET_ERROR == nSendSize) {
		err = WSAGetLastError();
		printf_s("\"sendto\" ����!, ������ %d\n", err);
		return -1;
	}
	int nReceiveSize = recv(connectSocketTcp, receiveBuff, MAX_BUF_LEN, 0);
	if (SOCKET_ERROR == nReceiveSize) {
		err = WSAGetLastError();
		printf_s("\"recvfrom\" ����!, ������ %d\n", err);
		return -1;
	}
	if (!strncmp("GOODBYE", receiveBuff,7))
	{
		cout << "�Ự����" << endl;
		closesocket(connectSocketTcp);
		return 0;
	}
	else
	{
		cout << "�˳���¼����" << endl;
		return -1;
	}
}
