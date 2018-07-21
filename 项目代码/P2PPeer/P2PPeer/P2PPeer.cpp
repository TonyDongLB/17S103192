// P2PPeer.cpp : 定义控制台应用程序的入口点。
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
	//启动peer服务端
	server.psConnection();
	int op = 0;
	int flag = 1;
	int logged = 0;
	char  path[MAX_FILE_LEN];
	while (flag)
	{
		cout << "请输入要进行的操作：" << endl;
		cout << "1.登录到服务器" << endl;
		cout << "2.添加共享文件" << endl;
		cout << "3.删除共享文件" << endl;
		cout << "4.列出所有可下载文件" << endl;
		cout << "5.下载指定共享文件" << endl;
		cout << "6.退出登录" << endl;
		cout << "7.关闭客户端" << endl;
		cin >> op;
		switch (op)
		{
		case 1:
			if (logged)
			{
				cout << "已经登录，不需再次登录：\n";
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
				cout << "当前未登录，请先登录到服务器：\n";
			}
			else
			{
				cout << "请输入要添加的文件完整路径：\n";
				cin >> path;
				//client.connectToCServ();
				client.addFile(path);
				//client.closeSession();
			}
			break;
		case 3:
			if (!logged)
			{
				cout << "当前未登录，请先登录到服务器：\n";
			}
			else
			{
				cout << "请输入要删除的文件完整路径：\n";
				cin >> path;
				//client.connectToCServ();
				client.deleteFile(path);
				//client.closeSession();
			}
			break;
		case 4:
			if (!logged)
			{
				cout << "当前未登录，请先登录到服务器：\n";
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
				cout << "当前未登录，请先登录到服务器：\n";
			}
			else
			{
				cout << "请输入要下载的文件名：\n";
				cin >> path;
				//client.connectToCServ();
				client.getFile(path);
				//client.closeSession();
			}
			break;
		case 6:
			if (!logged)
			{
				cout << "当前未登录，请先登录到服务器：\n";
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
