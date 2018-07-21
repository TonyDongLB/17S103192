// P2PFileServer.cpp : 定义控制台应用程序的入口点。
// 

#include "stdafx.h"
#include <WinSock2.h>
#include<ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <process.h>
#include "sqlite3.h" 
extern "C"{
#include "thpool.h"
}
#define DEFAULT_PORT 7777
#define QUEUE_LENGTH 32
#define NUM_THREADS 64
#define SERVER_NAME "Server"
#define OK_MSG "正常"
#define WARN_MSG "警告信息"
#define ERROR_MSG "错误信息"
#define USER_MSG "用户消息"
#define INFO_MSG "系统消息"
#define TP_UTIL 0.95
#define SIGHUP   1
#define MAX_PORT 10000
#define PRIVILEGED_PORT 1024
const char* DB_FILE = "E:/testp2p.db";
#pragma warning(disable:4996)
using namespace std;
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "sqlite3.lib")

typedef struct
{
	SOCKET fd;
	char ipaddr[128];
} p2p_t;
SOCKET loc_fd, inc_fd;

//客户端地址信息
struct sockaddr_in inc_addr;
socklen_t inc_len = sizeof(inc_addr);
thpool_t *thpool;
pthread_t net_thread;
int num_threads = NUM_THREADS;
int pidfile;
char *port;
int queue_length = QUEUE_LENGTH;
char clientaddr[128] = { '\0' };
sqlite3 *db;
time_t start_time;

static int c_count = 0;
void clean_string(char *);//字符串处理，去除诸如\b 之类的转义符
int client_count(int);//自加一，计算客户端数量
void console_help();//打印帮助信息
int recv_msg(int, char *);
int send_msg(int, char *);
int validate_int(char *);
void print_stats();
void stat_handler(int i=0);
void shutdown_handler(int i=0);
void *p2p(void *);
void *tcp_listen(void *);

int main(int argc, char* argv[])
{
	// 加载套接字库（必须）
	WORD wVersionRequested;
	WSADATA wsaData;
	// 套接字加载时错误提示
	int err;
	// 启动 socket api,版本 2.2
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	port = (char*)malloc(10 * sizeof(char));
	itoa(DEFAULT_PORT, port, 10);
	//======系统环境设置======
	//struct addrinfo hints, *result;
	// Service地址TCP
	SOCKADDR_IN sSercice;
	BOOL yes = TRUE;
	char command[512] = { '\0' };
	int i = 0;
	sqlite3_stmt *stmt;
	char query[256] = { '\0' };
	//调用 shutdown_handler 处理三种情况
	signal(SIGHUP, shutdown_handler);//关闭终端
	signal(SIGINT, shutdown_handler);//按下 CTRL+C
	signal(SIGTERM, shutdown_handler);//kill 命令
	// 注册自定义信号
	//signal(SIGUSR1, stat_handler);
	//signal(SIGUSR2, stat_handler);
	//term = strdup(ttyname(1));
	fprintf(stdout, "%s: %s 正在初始化 %s... \n", SERVER_NAME, INFO_MSG,
		SERVER_NAME);
	start_time = time(NULL);//开始计时
	//======处理执行参数======
	for (i = 1; i < argc; i++)
	{
		if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0)
		{
			fprintf(stdout, "usage: %s [-h | --help] [-p | --port port] [-q | --queue queue_length][-t | --threads thread_count]\n\n", SERVER_NAME);
				fprintf(stdout, "%s 参数说明:\n", SERVER_NAME);
			fprintf(stdout, "\t-h | --help: help - 展示帮助信息\n");
			fprintf(stdout, "\t-p | --port: port - 为服务器指定一个端口号(默认: %s)\n", DEFAULT_PORT);
				fprintf(stdout, "\t-q | --queue: queue_length - 为服务器指定连接队列的长度(默认: %d)\n", QUEUE_LENGTH);
				fprintf(stdout, "\t-t | --threads: thread_count - 为服务器指定连接池的长度(也就是最大支持的客户端数量)(默认: %d)\n", NUM_THREADS);
				fprintf(stdout, "\n");
			//退出
			exit(0);
		}
		else if (strcmp("-p", argv[i]) == 0 || strcmp("--port", argv[i]) == 0)
		{
			if (argv[i + 1] != NULL)
			{
				if (validate_int(argv[i + 1]))
				{
					if (atoi(argv[i + 1]) >= 0 && atoi(argv[i + 1]) <= MAX_PORT)
					{
						port = argv[i + 1];
						i++;
					}
					else
						fprintf(stderr, "%s: %s 端口号不在范围内(0-%d), 恢复默认端口号 %s\n", SERVER_NAME, ERROR_MSG, MAX_PORT, DEFAULT_PORT);
				}
				else
				{
					fprintf(stderr, "%s: %s 指定的端口号非法 , 恢复默认端口号 %s\n", SERVER_NAME, ERROR_MSG, DEFAULT_PORT);
				}
			}
			else
			{
				fprintf(stderr, "%s: %s 没有在 port 参数后找到端口值, 恢复默认端口号 %s\n", SERVER_NAME, ERROR_MSG, DEFAULT_PORT);
			}
		}
		else if (strcmp("-q", argv[i]) == 0 || strcmp("--queue", argv[i]) == 0)
		{
			if (argv[i + 1] != NULL)
			{
				if (validate_int(argv[i + 1]))
				{
					if (atoi(argv[i + 1]) >= 1)
					{
						queue_length = atoi(argv[i + 1]);
						i++;
					}
					else
						fprintf(stderr, "%s: %s 队列不能为非正数, 恢复默认队列长度 %d\n", SERVER_NAME, ERROR_MSG, QUEUE_LENGTH);
				}
				else
				{
					fprintf(stderr, "%s: %s 队列长度参数非法, 恢复默认队列长度 %d\n", SERVER_NAME, ERROR_MSG, QUEUE_LENGTH);
				}
			}
			else
			{
				// Print error and use default queue length if no length was specified after the flag
					fprintf(stderr, "%s: %s 没有在 queue 参数后找到队列长度, 恢复默认队列长度 %d\n", SERVER_NAME, ERROR_MSG, QUEUE_LENGTH);
			}
		}
		else if (strcmp("-t", argv[i]) == 0 || strcmp("--threads", argv[i]) == 0)
		{
			if (argv[i + 1] != NULL)
			{
				if (validate_int(argv[i + 1]))
				{
					if (atoi(argv[i + 1]) >= 1)
					{
						num_threads = atoi(argv[i + 1]);
						i++;
					}
					else
						fprintf(stderr, "%s: %s 线程数不能为非正数 , 恢复默认 %d 线程数\n", SERVER_NAME, ERROR_MSG, NUM_THREADS);
				}
				else
				{
					fprintf(stderr, "%s: %s 线程数参数非法, 恢复默认 %d 线程数\n", SERVER_NAME, ERROR_MSG, NUM_THREADS);
				}
			}
			else
			{
				fprintf(stderr, "%s: %s 没有在 thread 参数后找到线程数, 恢复默认 %d 线程数\n", SERVER_NAME, ERROR_MSG, NUM_THREADS);
			}
		}
		else
		{
			fprintf(stderr, "%s: %s 检测到未知参数'%s' , 输入 '%s -h' 查看 usage\n", SERVER_NAME, ERROR_MSG, argv[i], SERVER_NAME);
				exit(-1);
		}
	}
	//======准备数据库======
	sqlite3_open(DB_FILE, &db);
	if (db == NULL)
	{
		fprintf(stderr, "%s: %s sqlite: 不能打开 SQLite %s\n", SERVER_NAME,
			ERROR_MSG, DB_FILE);
		exit(-1);
	}
	sprintf(query, "DELETE FROM files");
	sqlite3_prepare_v2(db, query, strlen(query) + 1, &stmt, NULL);
	if (sqlite3_step(stmt) != SQLITE_DONE)
	{
		fprintf(stderr, "%s:sqlite: 操 作 失 败 ！ \n错误信息：%s", SERVER_NAME,
			sqlite3_errmsg(db));
		exit(-1);
	}
	sqlite3_finalize(stmt);
	//======初始化 TCP 连接======
	//memset(&hints, 0, sizeof(hints));
	//hints.ai_family = AF_INET;
	//hints.ai_socktype = SOCK_STREAM;
	//hints.ai_flags = AI_PASSIVE;
	sSercice.sin_family = AF_INET;
	//int a = atoi(port);
	sSercice.sin_port = htons(atoi(port));
	sSercice.sin_addr.s_addr = 0;
/*	if ((getaddrinfo(NULL, port, &hints, &result)) != 0)
	{
		fprintf(stderr, "%s: %s 调 用 getaddrinfo() 失 败 , 程 序 中 断 \n",
			SERVER_NAME, ERROR_MSG);
		exit(-1);
	}*/
	if ((loc_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		fprintf(stderr, "%s: %s socket 创建失败, 程序中断 \n", SERVER_NAME,
			ERROR_MSG);
		exit(-1);
	}
/*	if (setsockopt(loc_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int)) == SOCKET_ERROR)
	{
		fprintf(stderr, "%s: %s 不能允许 socket 重新绑定(SO_REUSEADDR), 程序中断 \n", SERVER_NAME, ERROR_MSG);
			exit(-1);
	}*/
	//绑定 socket
	if ((bind(loc_fd, (SOCKADDR*)&sSercice, sizeof(SOCKADDR))) == SOCKET_ERROR)
	{
		/*if (atoi(port) < PRIVILEGED_PORT)
			fprintf(stderr, "%s: %s 绑定 socket 失败，权限不足 \n", SERVER_NAME,
			ERROR_MSG);
		else
			fprintf(stderr, "%s: %s 绑定 socket失败，请检查当前端口是否被占用 \n",
			SERVER_NAME, ERROR_MSG);*/
		// Exit on failure
		printf("\"bind\" 错误! 错误码为 %d/n", WSAGetLastError());
		exit(-1);
	}
	//freeaddrinfo(result);
	listen(loc_fd, queue_length);//设置 socket 为 listen 模式
	//初始化一个线程池
	thpool = thpool_init(num_threads);
	pthread_create(&net_thread, NULL, &tcp_listen, NULL);
	fprintf(stdout, "%s: %s 服务器初始化成功 配置信息如下： [PID: %d] [端口号: %s][队列长度:%d][线程数:%d]\n", SERVER_NAME, OK_MSG, getpid(), port,
	   queue_length, num_threads);
	fprintf(stdout, "%s: %s 你 可 以 通 过 输 入 'help' 获 取 帮 助 信 息 \n",SERVER_NAME, INFO_MSG);
	//fprintf(stdout, "%s: %s 你可以通过输入'stop' 停止运行 \n", SERVER_NAME, INFO_MSG);
		//======用户输入处理======
		while (1)
		{
			fgets(command, sizeof(command), stdin);
			clean_string((char *)&command);
			if (strcmp(command, "clear") == 0)
				system("clear");
			else if (strcmp(command, "help") == 0)
				console_help();
			else if (strcmp(command, "stat") == 0)
				print_stats();
			else if (strcmp(command, "stop") == 0)
			{
				shutdown_handler();
				break;
			}
			else
				fprintf(stderr, "%s: %s 命 令 '%s' 未 知 , 输 入 'help' 获 取 帮 助 \n",
				SERVER_NAME, ERROR_MSG, command);
		}
		//关闭进程
		/*UINT code=0;
		int pid = _getpid();
		HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);;
		//LPVOID msg;
		TerminateProcess(handle, code);*/
/*		do
		{
			handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
			if (handle == NULL)
			{
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPSTR)&msg, 0, NULL);
				//printf("open pid:%s : %s\n", pid, msg);
				LocalFree(msg);
				continue;
			}

			if (!TerminateProcess(handle, code))
			{
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPSTR)&msg, 0, NULL);
				printf("kill pid:%s %s\n", pid, msg);
				LocalFree(msg);
				continue;
			}
		} while (pid = strtok(NULL, " "));*/
	//kill(_getpid(), SIGINT);
	free(port);
	port = NULL;
	return 0;
}

void clean_string(char *str)
{
	int i = 0;
	int index = 0;
	char buffer[1024];
	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] != '\b' && str[i] != '\n' && str[i] != '\r')
			buffer[index++] = str[i];
	}
	memset(str, 0, sizeof(str));
	buffer[index] = '\0';
	strcpy_s(str, strlen(buffer) + 1, buffer);
}

int client_count(int change)
{
	c_count += change;
	return c_count;
}

void console_help()
{
	fprintf(stdout, "%s 帮助:\n", SERVER_NAME);
	fprintf(stdout, "\tclear - 清除终端信息\n");
	fprintf(stdout, "\t help - 获取帮助信息\n");
	fprintf(stdout, "\t stat - 获取当前状态\n");
	fprintf(stdout, "\t stop - 停止服务器\n");
}

/*void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	else
		return &(((struct sockaddr_in6*)sa)->sin6_addr);
}*/

int recv_msg(SOCKET fd, char *message)
{
	int b_received = 0;
	int b_total = 0;
	char buffer[1024];
	memset(buffer, '\0', sizeof(buffer));
	b_received = recv(fd, buffer, sizeof(buffer), 0);
	b_total += b_received;
	strcpy_s(message, b_total + 1, buffer);
	return b_total;
}

int send_msg(SOCKET fd, char *message)
{
	int n = send(fd, message, strlen(message), 0);
	Sleep(10);
	return n;
}

int validate_int(char *string)
{
	int isInt = 1;
	int j = 0;
	for (j = 0; j < strlen(string); j++)
	{
		if (isInt == 1)
		{
			if (!isdigit(string[j]))
				isInt = 0;
		}
	}
	return isInt;
}

void print_stats()
{
	//打印运行时间
	int hours, minutes, seconds;
	char runtime[32] = { '\0' };
	char tpusage[32] = { '\0' };
	seconds = (int)difftime(time(NULL), start_time);
	minutes = seconds / 60;
	hours = minutes / 60;
	minutes = minutes % 60;
	seconds = seconds % 60;
	sprintf_s(runtime, "%02d:%02d:%02d", hours, minutes, seconds);
	//打印连接池状态
	//连接池容量绰绰有余时
	if (client_count(0) < (num_threads * TP_UTIL))
	{
		fprintf(stdout, "%s: %s ", SERVER_NAME, OK_MSG);
		sprintf_s(tpusage, "[在线用户数: %d/%d]", client_count(0), num_threads);
	}
	// 连接池快满了或者已经饱和时
	else if (((double)client_count(0) >= ((double)num_threads * TP_UTIL)) &&
		client_count(0) <= num_threads)
	{
		//转为警告
		fprintf(stdout, "%s: %s ", SERVER_NAME, WARN_MSG);
		sprintf_s(tpusage, "\033[1;33m[在线用户数: %d/%d]\033[0m", client_count(0),num_threads);
	}
	// 连接池已经超负荷时
	else
	{
		// 转为错误
		fprintf(stdout, "%s: %s ", SERVER_NAME, ERROR_MSG);
		sprintf_s(tpusage, "\033[1;31m[在线用户数: %d/%d]\033[0m", client_count(0),num_threads);
	}
	fprintf(stdout, "服务器运行中： [PID: %d] [运行时间: %s] [运行端口: %s][队列长度:%d] % s\n", _getpid(), runtime, port, queue_length, tpusage);
}
// 当产生 SIGUSR1/SIGUSR2 信号时，向客户端报告服务器状态
void stat_handler(int i)
{
	//freopen(term, "w", stdout);
	// 打印服务器状态
	print_stats();
	// Return stdout to /dev/null
	//fclose(stdout);
}

void shutdown_handler(int i)
{
	// 关闭 net_thread，停止接收新的请求
	pthread_cancel(net_thread);
	fprintf(stdout, "\n");
	// 关闭 SQLite 数据库
	if (sqlite3_close(db) != SQLITE_OK)
	{
		// 失败时
		fprintf(stderr, "%s: %s sqlite: 未能关闭 SQLite 数据库.\n", SERVER_NAME,
			ERROR_MSG);
		exit(-1);
	}
	// 尝试从容关闭 socket
	/*if (shutdown(loc_fd, 2) == -1)
	{
		// 失败时
		fprintf(stderr, "%s: %s 错误号：%d 未能成功 shutdown本机的 socket.\n", SERVER_NAME, ERROR_MSG, WSAGetLastError());
		exit(-1);
	}*/
	// 关闭 socket
	if (closesocket(loc_fd) == -1)
	{
		// 失败时
		fprintf(stderr, "%s: %s 未能成功 close 本机的 socket.\n", SERVER_NAME,
			ERROR_MSG);
		exit(-1);
	}
	// 关闭所有创建的连接池
	thpool_destroy(thpool);
	fprintf(stdout, "%s: %s 成 功 剔 除 %d 台 客 户 端 设 备 ， 服 务 器 中 断 。 \n",
		SERVER_NAME, OK_MSG, client_count(0));
	exit(0);
}
void *p2p(void *args)
{
	char in[512], out[512] = { '\0' };
	p2p_t params = *((p2p_t *)(args));
	char *filename, *filehash, *filesize;
	long int f_size = 0;
	char peeraddr[128] = { '\0' };
	strcpy(peeraddr, params.ipaddr);
	SOCKET user_fd = params.fd;
	char query[256];
	int status;
	int flag = 0;
	sqlite3_stmt *stmt;
	//sprintf(out, "%s: %s \n", SERVER_NAME, USER_MSG);
	//send_msg(user_fd, out);
	// 等待客户端发来消息
	while ((strcmp(in, "CONNECT")) != 0 && (strcmp(in, "QUIT") != 0))
	{
		//获取消息
		recv_msg(user_fd, (char *)&in);
		clean_string((char *)&in);
		//如果发来的是握手消息 CONNECT，返回确认信息 ACCEPT
		if (strcmp(in, "CONNECT") == 0)
		{
			fprintf(stdout, "%s: %s 检测到 %s 向服务器发送了一个握手消息，返回确认消息[句柄:%d]\n", SERVER_NAME, OK_MSG, peeraddr, user_fd);
			sprintf(out, "ACCEPT");
			send_msg(user_fd, out);
		}
	}
	//服务端已经发送确认信息，等待客户端发来进一步的消息
	while (strcmp(in, "QUIT") != 0)
	{
		memset(in, 0, sizeof(in));
		memset(out, 0, sizeof(out));
		memset(query, 0, sizeof(query));
		//获取消息
		recv_msg(user_fd, (char *)&in);
		clean_string((char *)&in);
		// 格式: ADD <文件名> <Hash 值> <文件大小>
		if (strncmp(in, "ADD", 3) == 0)
		{
			strtok(in, " ");
			filename = strtok(NULL, " ");
			flag = 0;
			if (filename != NULL)
			{
				filehash = strtok(NULL, " ");
				if (filehash != NULL)
				{
					filesize = strtok(NULL, " ");
					if ((filesize != NULL) && (validate_int(filesize) == 1))
					{
						f_size = atoi(filesize);
						sprintf(query, "INSERT INTO files VALUES('%s', '%s','%ld', '%s')", filename, filehash, f_size, peeraddr);
						sqlite3_prepare_v2(db, query, strlen(query) + 1, &stmt, NULL);
						if ((status = sqlite3_step(stmt)) != SQLITE_DONE)
						{
							if (status == SQLITE_CONSTRAINT)
							{
								fprintf(stderr, "%s: %s sqlite: 添加文件失败，服务器数据库中已经存在当前文件\n", SERVER_NAME, ERROR_MSG);
								sprintf(out, "ERROR 添加文件失败，服务器数据库中已经存在当前文件\n");
								send_msg(user_fd, out);
							}
							else
							{
								fprintf(stderr, "%s: %s sqlite: 添加文件失败 \n",
									SERVER_NAME, ERROR_MSG);
								sprintf(out, "ERROR 添加文件信息到数据库失败，错误%s:\n",sqlite3_errmsg(db));
								send_msg(user_fd, out);
							}
						}
						sqlite3_finalize(stmt);
						if (status == SQLITE_DONE)
						{
							fprintf(stdout, "%s: %s 客户端%s 向服务器添加了文件 % 20s[hash 值 : % 20s][大小:% 10ld]\n", SERVER_NAME, INFO_MSG, peeraddr, filename, filehash, f_size);
							//返回 OK
							sprintf(out, "OK\n");
							send_msg(user_fd, out);
						}
					}
					else
						flag = 1;
				}
				else
					flag = 1;
			}
			else
				flag = 1;
			//传入参数的格式错误
			if (flag)
			{
				fprintf(stderr, "%s: %s 添加文件失败，传入参数的格式错误 \n", SERVER_NAME, ERROR_MSG);
				sprintf(out, "ERROR 添加文件失败，传入参数的格式错误\n");
				send_msg(user_fd, out);
			}
			
		}
		// 格式: DELETE [文件名] [HASH 值]
		else if (strncmp(in, "DELETE", 6) == 0)
		{
			strtok(in, " ");
			filename = strtok(NULL, " ");
			flag = 0;
			if (filename != NULL)
			{
				filehash = strtok(NULL, " ");
				if (filehash != NULL)
				{
					sprintf(query, "DELETE FROM files WHERE filename='%s' AND filehash = '%s' AND peeraddr = '%s'", filename, filehash, peeraddr);
					sqlite3_prepare_v2(db, query, strlen(query) + 1, &stmt, NULL);
					if (sqlite3_step(stmt) != SQLITE_DONE)
					{
						fprintf(stderr, "%s: %s sqlite: 删 除 文 件 失 败 \n",
							SERVER_NAME, ERROR_MSG);
						sprintf(out, "ERROR 从数据库中删除文件失败，原因未知\n");
						send_msg(user_fd, out);
					}
					sqlite3_finalize(stmt);
					fprintf(stdout, "%s: %s 客 户 端 %s 向 服 务 器 删 除 了 文 件'%s'('%s') \n", SERVER_NAME, OK_MSG, peeraddr, filename, filehash);
					sprintf(out, "OK\n");
					send_msg(user_fd, out);
				}
				else
					flag = 1;
			}
			else
				flag = 1;
			//传入参数的格式错误
			if (flag)
			{
				fprintf(stderr, "%s: %s 删除文件失败，传入参数的格式错误 \n",
					SERVER_NAME, ERROR_MSG);
				sprintf(out, "ERROR 删除文件失败，传入参数的格式错误\n");
				send_msg(user_fd, out);
			}
			
		}
		// LIST
		else if (strcmp(in, "LIST") == 0)
		{
			sprintf(query, "SELECT DISTINCT filename,f_size,peeraddr FROM files ORDER BY filename ASC");
			sqlite3_prepare_v2(db, query, strlen(query) + 1, &stmt, NULL);
			while ((status = sqlite3_step(stmt)) != SQLITE_DONE)
			{
				if (status == SQLITE_ERROR)
				{
					fprintf(stderr, "%s: %s sqlite: 未能获得所有记录，数据库错误\n", SERVER_NAME, ERROR_MSG);
					sprintf(out, "ERROR 未能获得所有记录，服务端数据库错误\n");
					send_msg(user_fd, out);
				}
				else /*if (strcmp(peeraddr, (char *)sqlite3_column_text(stmt, 2)))*/
				{
					sprintf(out, "%s %d", sqlite3_column_text(stmt, 0), sqlite3_column_int(stmt, 1));
					send_msg(user_fd, out);
				}
			}
			sqlite3_finalize(stmt);
			sprintf(out, "OK");
			send_msg(user_fd, out);
			
		}
		// QUIT
		else if (strcmp(in, "QUIT") == 0)
		{
			continue;
		}
		// syntax: REQUEST [文件名]
		else if (strncmp(in, "REQUEST", 7) == 0)
		{
			strtok(in, " ");
			filename = strtok(NULL, " ");
			if (filename != NULL)
			{
				sprintf(query, "SELECT filename,peeraddr,f_size FROM files WHERE filename Like '%%%s' ORDER BY peeraddr ASC", filename);
				sqlite3_prepare_v2(db, query, strlen(query) + 1, &stmt, NULL);
				while ((status = sqlite3_step(stmt)) != SQLITE_DONE)
				{
					if (status == SQLITE_ERROR)
					{
						fprintf(stderr, "%s: %s sqlite: 未能成功获取文件信息，数据库错误 '%s'\n", SERVER_NAME, ERROR_MSG, filename);
						sprintf(out, "ERROR 未能成功获取文件信息，数据库错误\n");
						send_msg(user_fd, out);
					}
					else
					{
						sprintf(out, "%s %s %ld", sqlite3_column_text(stmt, 0), sqlite3_column_text(stmt, 1), (long int)sqlite3_column_int(stmt, 2));
						send_msg(user_fd, out);
					}
				}
				sqlite3_finalize(stmt);
				sprintf(out, "OK\n");
				send_msg(user_fd, out);
			}
			else
			{
				sprintf(out, "ERROR 没能成功获得请求的文件名 \n");
				send_msg(user_fd, out);
			}
			
		}
		else
		{
			sprintf(out, "ERROR 参数错误\n");
			send_msg(user_fd, out);
			
		}
	}
	memset(out, 0, sizeof(out));
	sprintf(out, "GOODBYE\n");
	send_msg(user_fd, out);
	fprintf(stdout, "%s: %s 客 户 端 %s 已 经 从 服 务 器 注 销 登 录 [ 在 线 用 户数 : %d / %d]\n", SERVER_NAME, OK_MSG, peeraddr, client_count(-1), NUM_THREADS);
	sprintf(query, "DELETE FROM files WHERE peeraddr='%s'", peeraddr);
	sqlite3_prepare_v2(db, query, strlen(query) + 1, &stmt, NULL);
	if (sqlite3_step(stmt) != SQLITE_DONE)
	{
		fprintf(stderr, "%s: %s 客 户 端 %s 剔 除 失 败 [ 句 柄 : %d]\n",
			SERVER_NAME, ERROR_MSG, peeraddr, user_fd);
		return (void *)-1;
	}
	sqlite3_finalize(stmt);
	if (closesocket(user_fd) == -1)
	{
		fprintf(stderr, "%s: %s 关闭套接字失败 [句柄: %d]\n", SERVER_NAME,
			ERROR_MSG, user_fd);
		return (void *)-1;
	}
	return (void *)0;
}

//建立 TCP 连接
void *tcp_listen(void* v)
{
	p2p_t params;
	char out[512] = { '\0' };
	while (1)
	{
		if ((inc_fd = accept(loc_fd, (struct sockaddr *)&inc_addr, &inc_len)) == -1)
		{
			fprintf(stderr, "%s: %s 未 能 成 功 接 收 连 接 \n", SERVER_NAME,
				ERROR_MSG);
			return (void *)-1;
		}
		else
		{
			//inet_ntop(inc_addr.sin_family, inc_addr.sin_addr,clientaddr, sizeof(clientaddr));
			fprintf(stdout, "%s: %s 监测到 %s 正在尝试连接到服务器 [socket 编号: %d][在线用户数:%d / %d]\n", SERVER_NAME, INFO_MSG, inet_ntoa(inc_addr.sin_addr), inc_fd, client_count(1), num_threads);
			if (((double)client_count(0) >= ((double)num_threads * TP_UTIL)) &&(client_count(0) <= num_threads))
			{
				if (client_count(0) == num_threads)
					fprintf(stdout, "%s: %s 连 接 池 资 源 耗 尽 [ 在 线 用 户数: %d / %d]\n", SERVER_NAME, WARN_MSG, client_count(0), num_threads);
				else
					fprintf(stdout, "%s: %s 连 接 池 资 源 即 将 耗 尽 [ 在 线 用 户数: %d / %d]\n", SERVER_NAME, WARN_MSG, client_count(0), num_threads);
			}
			else if ((client_count(0)) > num_threads)
			{
				fprintf(stderr, "%s: %s 连接池资源耗尽，仍然有新用户尝试连接[在 线 用 户 数 : %d / %d]\n", SERVER_NAME, ERROR_MSG, client_count(0),num_threads);
				sprintf(out, "%s: %s 服 务 器 负 载 过 大 , 请 稍 后 再 试 \n",SERVER_NAME, USER_MSG);
				send_msg(inc_fd, out);
			}
			params.fd = inc_fd;
			strcpy(params.ipaddr, inet_ntoa(inc_addr.sin_addr));
			thpool_add_work(thpool, &p2p, (void*)&params);//添加到线程池
		}
	}
}