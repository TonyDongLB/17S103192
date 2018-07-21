// P2PPeer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include "PeerServer.h"
#include "PeerClient.h"
using namespace std;


int main(int argc, char* argv[])
{
	PeerServer server;
	PeerClient client;
	//����peer�����
	server.psConnection();
	int op = 0;
	int flag = 1;
	int logged = 0;
	char  path[MAX_FILE_LEN];
	while (flag)
	{
		cout << "������Ҫ���еĲ�����" << endl;
		cout << "1.��¼��������" << endl;
		cout << "2.��ӹ����ļ�" << endl;
		cout << "3.ɾ�������ļ�" << endl;
		cout << "4.�г����п������ļ�" << endl;
		cout << "5.����ָ�������ļ�" << endl;
		cout << "6.�˳���¼" << endl;
		cout << "7.�رտͻ���" << endl;
		cin >> op;
		switch (op)
		{
		case 1:
			if (logged)
			{
				cout << "�Ѿ���¼�������ٴε�¼��\n";
			}
			else
			{
				client.connectToCServ();
				logged = 1;
			}
			break;
		case 2:
			if (!logged)
			{
				cout << "��ǰδ��¼�����ȵ�¼����������\n";
			}
			else
			{
				cout << "������Ҫ��ӵ��ļ�����·����\n";
				cin >> path;
				//client.connectToCServ();
				client.addFile(path);
				//client.closeSession();
			}
			break;
		case 3:
			if (!logged)
			{
				cout << "��ǰδ��¼�����ȵ�¼����������\n";
			}
			else
			{
				cout << "������Ҫɾ�����ļ�����·����\n";
				cin >> path;
				//client.connectToCServ();
				client.deleteFile(path);
				//client.closeSession();
			}
			break;
		case 4:
			if (!logged)
			{
				cout << "��ǰδ��¼�����ȵ�¼����������\n";
			}
			else
			{
				//client.connectToCServ();
				client.showFileList();
				//client.closeSession();
			}
			break;
		case 5:
			if (!logged)
			{
				cout << "��ǰδ��¼�����ȵ�¼����������\n";
			}
			else
			{
				cout << "������Ҫ���ص��ļ�����\n";
				cin >> path;
				//client.connectToCServ();
				client.getFile(path);
				//client.closeSession();
			}
			break;
		case 6:
			if (!logged)
			{
				cout << "��ǰδ��¼�����ȵ�¼����������\n";
			}
			else
			{
				logged = 0;
				flag = 1;
				//client.connectToCServ();
				client.quitFromSer();
				//client.closeSession();
			}
			break;
		case 7:
			if (!logged)
			{
				flag = 0;
			}
			else
			{
				logged = 0;
				flag = 0;
				//client.connectToCServ();
				client.quitFromSer();
				//client.closeSession();
			}
			break;
		default:
			break;
		}
	}
	return 0;
}
