// P2PFileServer.cpp : �������̨Ӧ�ó������ڵ㡣
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
#define OK_MSG "����"
#define WARN_MSG "������Ϣ"
#define ERROR_MSG "������Ϣ"
#define USER_MSG "�û���Ϣ"
#define INFO_MSG "ϵͳ��Ϣ"
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

//�ͻ��˵�ַ��Ϣ
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
void clean_string(char *);//�ַ�������ȥ������\b ֮���ת���
int client_count(int);//�Լ�һ������ͻ�������
void console_help();//��ӡ������Ϣ
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
	// �����׽��ֿ⣨���룩
	WORD wVersionRequested;
	WSADATA wsaData;
	// �׽��ּ���ʱ������ʾ
	int err;
	// ���� socket api,�汾 2.2
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	port = (char*)malloc(10 * sizeof(char));
	itoa(DEFAULT_PORT, port, 10);
	//======ϵͳ��������======
	//struct addrinfo hints, *result;
	// Service��ַTCP
	SOCKADDR_IN sSercice;
	BOOL yes = TRUE;
	char command[512] = { '\0' };
	int i = 0;
	sqlite3_stmt *stmt;
	char query[256] = { '\0' };
	//���� shutdown_handler �����������
	signal(SIGHUP, shutdown_handler);//�ر��ն�
	signal(SIGINT, shutdown_handler);//���� CTRL+C
	signal(SIGTERM, shutdown_handler);//kill ����
	// ע���Զ����ź�
	//signal(SIGUSR1, stat_handler);
	//signal(SIGUSR2, stat_handler);
	//term = strdup(ttyname(1));
	fprintf(stdout, "%s: %s ���ڳ�ʼ�� %s... \n", SERVER_NAME, INFO_MSG,
		SERVER_NAME);
	start_time = time(NULL);//��ʼ��ʱ
	//======����ִ�в���======
	for (i = 1; i < argc; i++)
	{
		if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0)
		{
			fprintf(stdout, "usage: %s [-h | --help] [-p | --port port] [-q | --queue queue_length][-t | --threads thread_count]\n\n", SERVER_NAME);
				fprintf(stdout, "%s ����˵��:\n", SERVER_NAME);
			fprintf(stdout, "\t-h | --help: help - չʾ������Ϣ\n");
			fprintf(stdout, "\t-p | --port: port - Ϊ������ָ��һ���˿ں�(Ĭ��: %s)\n", DEFAULT_PORT);
				fprintf(stdout, "\t-q | --queue: queue_length - Ϊ������ָ�����Ӷ��еĳ���(Ĭ��: %d)\n", QUEUE_LENGTH);
				fprintf(stdout, "\t-t | --threads: thread_count - Ϊ������ָ�����ӳصĳ���(Ҳ�������֧�ֵĿͻ�������)(Ĭ��: %d)\n", NUM_THREADS);
				fprintf(stdout, "\n");
			//�˳�
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
						fprintf(stderr, "%s: %s �˿ںŲ��ڷ�Χ��(0-%d), �ָ�Ĭ�϶˿ں� %s\n", SERVER_NAME, ERROR_MSG, MAX_PORT, DEFAULT_PORT);
				}
				else
				{
					fprintf(stderr, "%s: %s ָ���Ķ˿ںŷǷ� , �ָ�Ĭ�϶˿ں� %s\n", SERVER_NAME, ERROR_MSG, DEFAULT_PORT);
				}
			}
			else
			{
				fprintf(stderr, "%s: %s û���� port �������ҵ��˿�ֵ, �ָ�Ĭ�϶˿ں� %s\n", SERVER_NAME, ERROR_MSG, DEFAULT_PORT);
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
						fprintf(stderr, "%s: %s ���в���Ϊ������, �ָ�Ĭ�϶��г��� %d\n", SERVER_NAME, ERROR_MSG, QUEUE_LENGTH);
				}
				else
				{
					fprintf(stderr, "%s: %s ���г��Ȳ����Ƿ�, �ָ�Ĭ�϶��г��� %d\n", SERVER_NAME, ERROR_MSG, QUEUE_LENGTH);
				}
			}
			else
			{
				// Print error and use default queue length if no length was specified after the flag
					fprintf(stderr, "%s: %s û���� queue �������ҵ����г���, �ָ�Ĭ�϶��г��� %d\n", SERVER_NAME, ERROR_MSG, QUEUE_LENGTH);
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
						fprintf(stderr, "%s: %s �߳�������Ϊ������ , �ָ�Ĭ�� %d �߳���\n", SERVER_NAME, ERROR_MSG, NUM_THREADS);
				}
				else
				{
					fprintf(stderr, "%s: %s �߳��������Ƿ�, �ָ�Ĭ�� %d �߳���\n", SERVER_NAME, ERROR_MSG, NUM_THREADS);
				}
			}
			else
			{
				fprintf(stderr, "%s: %s û���� thread �������ҵ��߳���, �ָ�Ĭ�� %d �߳���\n", SERVER_NAME, ERROR_MSG, NUM_THREADS);
			}
		}
		else
		{
			fprintf(stderr, "%s: %s ��⵽δ֪����'%s' , ���� '%s -h' �鿴 usage\n", SERVER_NAME, ERROR_MSG, argv[i], SERVER_NAME);
				exit(-1);
		}
	}
	//======׼�����ݿ�======
	sqlite3_open(DB_FILE, &db);
	if (db == NULL)
	{
		fprintf(stderr, "%s: %s sqlite: ���ܴ� SQLite %s\n", SERVER_NAME,
			ERROR_MSG, DB_FILE);
		exit(-1);
	}
	sprintf(query, "DELETE FROM files");
	sqlite3_prepare_v2(db, query, strlen(query) + 1, &stmt, NULL);
	if (sqlite3_step(stmt) != SQLITE_DONE)
	{
		fprintf(stderr, "%s:sqlite: �� �� ʧ �� �� \n������Ϣ��%s", SERVER_NAME,
			sqlite3_errmsg(db));
		exit(-1);
	}
	sqlite3_finalize(stmt);
	//======��ʼ�� TCP ����======
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
		fprintf(stderr, "%s: %s �� �� getaddrinfo() ʧ �� , �� �� �� �� \n",
			SERVER_NAME, ERROR_MSG);
		exit(-1);
	}*/
	if ((loc_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		fprintf(stderr, "%s: %s socket ����ʧ��, �����ж� \n", SERVER_NAME,
			ERROR_MSG);
		exit(-1);
	}
/*	if (setsockopt(loc_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int)) == SOCKET_ERROR)
	{
		fprintf(stderr, "%s: %s �������� socket ���°�(SO_REUSEADDR), �����ж� \n", SERVER_NAME, ERROR_MSG);
			exit(-1);
	}*/
	//�� socket
	if ((bind(loc_fd, (SOCKADDR*)&sSercice, sizeof(SOCKADDR))) == SOCKET_ERROR)
	{
		/*if (atoi(port) < PRIVILEGED_PORT)
			fprintf(stderr, "%s: %s �� socket ʧ�ܣ�Ȩ�޲��� \n", SERVER_NAME,
			ERROR_MSG);
		else
			fprintf(stderr, "%s: %s �� socketʧ�ܣ����鵱ǰ�˿��Ƿ�ռ�� \n",
			SERVER_NAME, ERROR_MSG);*/
		// Exit on failure
		printf("\"bind\" ����! ������Ϊ %d/n", WSAGetLastError());
		exit(-1);
	}
	//freeaddrinfo(result);
	listen(loc_fd, queue_length);//���� socket Ϊ listen ģʽ
	//��ʼ��һ���̳߳�
	thpool = thpool_init(num_threads);
	pthread_create(&net_thread, NULL, &tcp_listen, NULL);
	fprintf(stdout, "%s: %s ��������ʼ���ɹ� ������Ϣ���£� [PID: %d] [�˿ں�: %s][���г���:%d][�߳���:%d]\n", SERVER_NAME, OK_MSG, getpid(), port,
	   queue_length, num_threads);
	fprintf(stdout, "%s: %s �� �� �� ͨ �� �� �� 'help' �� ȡ �� �� �� Ϣ \n",SERVER_NAME, INFO_MSG);
	//fprintf(stdout, "%s: %s �����ͨ������'stop' ֹͣ���� \n", SERVER_NAME, INFO_MSG);
		//======�û����봦��======
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
				fprintf(stderr, "%s: %s �� �� '%s' δ ֪ , �� �� 'help' �� ȡ �� �� \n",
				SERVER_NAME, ERROR_MSG, command);
		}
		//�رս���
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
	fprintf(stdout, "%s ����:\n", SERVER_NAME);
	fprintf(stdout, "\tclear - ����ն���Ϣ\n");
	fprintf(stdout, "\t help - ��ȡ������Ϣ\n");
	fprintf(stdout, "\t stat - ��ȡ��ǰ״̬\n");
	fprintf(stdout, "\t stop - ֹͣ������\n");
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
	//��ӡ����ʱ��
	int hours, minutes, seconds;
	char runtime[32] = { '\0' };
	char tpusage[32] = { '\0' };
	seconds = (int)difftime(time(NULL), start_time);
	minutes = seconds / 60;
	hours = minutes / 60;
	minutes = minutes % 60;
	seconds = seconds % 60;
	sprintf_s(runtime, "%02d:%02d:%02d", hours, minutes, seconds);
	//��ӡ���ӳ�״̬
	//���ӳ������´�����ʱ
	if (client_count(0) < (num_threads * TP_UTIL))
	{
		fprintf(stdout, "%s: %s ", SERVER_NAME, OK_MSG);
		sprintf_s(tpusage, "[�����û���: %d/%d]", client_count(0), num_threads);
	}
	// ���ӳؿ����˻����Ѿ�����ʱ
	else if (((double)client_count(0) >= ((double)num_threads * TP_UTIL)) &&
		client_count(0) <= num_threads)
	{
		//תΪ����
		fprintf(stdout, "%s: %s ", SERVER_NAME, WARN_MSG);
		sprintf_s(tpusage, "\033[1;33m[�����û���: %d/%d]\033[0m", client_count(0),num_threads);
	}
	// ���ӳ��Ѿ�������ʱ
	else
	{
		// תΪ����
		fprintf(stdout, "%s: %s ", SERVER_NAME, ERROR_MSG);
		sprintf_s(tpusage, "\033[1;31m[�����û���: %d/%d]\033[0m", client_count(0),num_threads);
	}
	fprintf(stdout, "�����������У� [PID: %d] [����ʱ��: %s] [���ж˿�: %s][���г���:%d] % s\n", _getpid(), runtime, port, queue_length, tpusage);
}
// ������ SIGUSR1/SIGUSR2 �ź�ʱ����ͻ��˱��������״̬
void stat_handler(int i)
{
	//freopen(term, "w", stdout);
	// ��ӡ������״̬
	print_stats();
	// Return stdout to /dev/null
	//fclose(stdout);
}

void shutdown_handler(int i)
{
	// �ر� net_thread��ֹͣ�����µ�����
	pthread_cancel(net_thread);
	fprintf(stdout, "\n");
	// �ر� SQLite ���ݿ�
	if (sqlite3_close(db) != SQLITE_OK)
	{
		// ʧ��ʱ
		fprintf(stderr, "%s: %s sqlite: δ�ܹر� SQLite ���ݿ�.\n", SERVER_NAME,
			ERROR_MSG);
		exit(-1);
	}
	// ���Դ��ݹر� socket
	/*if (shutdown(loc_fd, 2) == -1)
	{
		// ʧ��ʱ
		fprintf(stderr, "%s: %s ����ţ�%d δ�ܳɹ� shutdown������ socket.\n", SERVER_NAME, ERROR_MSG, WSAGetLastError());
		exit(-1);
	}*/
	// �ر� socket
	if (closesocket(loc_fd) == -1)
	{
		// ʧ��ʱ
		fprintf(stderr, "%s: %s δ�ܳɹ� close ������ socket.\n", SERVER_NAME,
			ERROR_MSG);
		exit(-1);
	}
	// �ر����д��������ӳ�
	thpool_destroy(thpool);
	fprintf(stdout, "%s: %s �� �� �� �� %d ̨ �� �� �� �� �� �� �� �� �� �� �� �� \n",
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
	// �ȴ��ͻ��˷�����Ϣ
	while ((strcmp(in, "CONNECT")) != 0 && (strcmp(in, "QUIT") != 0))
	{
		//��ȡ��Ϣ
		recv_msg(user_fd, (char *)&in);
		clean_string((char *)&in);
		//�����������������Ϣ CONNECT������ȷ����Ϣ ACCEPT
		if (strcmp(in, "CONNECT") == 0)
		{
			fprintf(stdout, "%s: %s ��⵽ %s �������������һ��������Ϣ������ȷ����Ϣ[���:%d]\n", SERVER_NAME, OK_MSG, peeraddr, user_fd);
			sprintf(out, "ACCEPT");
			send_msg(user_fd, out);
		}
	}
	//������Ѿ�����ȷ����Ϣ���ȴ��ͻ��˷�����һ������Ϣ
	while (strcmp(in, "QUIT") != 0)
	{
		memset(in, 0, sizeof(in));
		memset(out, 0, sizeof(out));
		memset(query, 0, sizeof(query));
		//��ȡ��Ϣ
		recv_msg(user_fd, (char *)&in);
		clean_string((char *)&in);
		// ��ʽ: ADD <�ļ���> <Hash ֵ> <�ļ���С>
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
								fprintf(stderr, "%s: %s sqlite: ����ļ�ʧ�ܣ����������ݿ����Ѿ����ڵ�ǰ�ļ�\n", SERVER_NAME, ERROR_MSG);
								sprintf(out, "ERROR ����ļ�ʧ�ܣ����������ݿ����Ѿ����ڵ�ǰ�ļ�\n");
								send_msg(user_fd, out);
							}
							else
							{
								fprintf(stderr, "%s: %s sqlite: ����ļ�ʧ�� \n",
									SERVER_NAME, ERROR_MSG);
								sprintf(out, "ERROR ����ļ���Ϣ�����ݿ�ʧ�ܣ�����%s:\n",sqlite3_errmsg(db));
								send_msg(user_fd, out);
							}
						}
						sqlite3_finalize(stmt);
						if (status == SQLITE_DONE)
						{
							fprintf(stdout, "%s: %s �ͻ���%s �������������ļ� % 20s[hash ֵ : % 20s][��С:% 10ld]\n", SERVER_NAME, INFO_MSG, peeraddr, filename, filehash, f_size);
							//���� OK
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
			//��������ĸ�ʽ����
			if (flag)
			{
				fprintf(stderr, "%s: %s ����ļ�ʧ�ܣ���������ĸ�ʽ���� \n", SERVER_NAME, ERROR_MSG);
				sprintf(out, "ERROR ����ļ�ʧ�ܣ���������ĸ�ʽ����\n");
				send_msg(user_fd, out);
			}
			
		}
		// ��ʽ: DELETE [�ļ���] [HASH ֵ]
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
						fprintf(stderr, "%s: %s sqlite: ɾ �� �� �� ʧ �� \n",
							SERVER_NAME, ERROR_MSG);
						sprintf(out, "ERROR �����ݿ���ɾ���ļ�ʧ�ܣ�ԭ��δ֪\n");
						send_msg(user_fd, out);
					}
					sqlite3_finalize(stmt);
					fprintf(stdout, "%s: %s �� �� �� %s �� �� �� �� ɾ �� �� �� ��'%s'('%s') \n", SERVER_NAME, OK_MSG, peeraddr, filename, filehash);
					sprintf(out, "OK\n");
					send_msg(user_fd, out);
				}
				else
					flag = 1;
			}
			else
				flag = 1;
			//��������ĸ�ʽ����
			if (flag)
			{
				fprintf(stderr, "%s: %s ɾ���ļ�ʧ�ܣ���������ĸ�ʽ���� \n",
					SERVER_NAME, ERROR_MSG);
				sprintf(out, "ERROR ɾ���ļ�ʧ�ܣ���������ĸ�ʽ����\n");
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
					fprintf(stderr, "%s: %s sqlite: δ�ܻ�����м�¼�����ݿ����\n", SERVER_NAME, ERROR_MSG);
					sprintf(out, "ERROR δ�ܻ�����м�¼����������ݿ����\n");
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
		// syntax: REQUEST [�ļ���]
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
						fprintf(stderr, "%s: %s sqlite: δ�ܳɹ���ȡ�ļ���Ϣ�����ݿ���� '%s'\n", SERVER_NAME, ERROR_MSG, filename);
						sprintf(out, "ERROR δ�ܳɹ���ȡ�ļ���Ϣ�����ݿ����\n");
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
				sprintf(out, "ERROR û�ܳɹ����������ļ��� \n");
				send_msg(user_fd, out);
			}
			
		}
		else
		{
			sprintf(out, "ERROR ��������\n");
			send_msg(user_fd, out);
			
		}
	}
	memset(out, 0, sizeof(out));
	sprintf(out, "GOODBYE\n");
	send_msg(user_fd, out);
	fprintf(stdout, "%s: %s �� �� �� %s �� �� �� �� �� �� ע �� �� ¼ [ �� �� �� ���� : %d / %d]\n", SERVER_NAME, OK_MSG, peeraddr, client_count(-1), NUM_THREADS);
	sprintf(query, "DELETE FROM files WHERE peeraddr='%s'", peeraddr);
	sqlite3_prepare_v2(db, query, strlen(query) + 1, &stmt, NULL);
	if (sqlite3_step(stmt) != SQLITE_DONE)
	{
		fprintf(stderr, "%s: %s �� �� �� %s �� �� ʧ �� [ �� �� : %d]\n",
			SERVER_NAME, ERROR_MSG, peeraddr, user_fd);
		return (void *)-1;
	}
	sqlite3_finalize(stmt);
	if (closesocket(user_fd) == -1)
	{
		fprintf(stderr, "%s: %s �ر��׽���ʧ�� [���: %d]\n", SERVER_NAME,
			ERROR_MSG, user_fd);
		return (void *)-1;
	}
	return (void *)0;
}

//���� TCP ����
void *tcp_listen(void* v)
{
	p2p_t params;
	char out[512] = { '\0' };
	while (1)
	{
		if ((inc_fd = accept(loc_fd, (struct sockaddr *)&inc_addr, &inc_len)) == -1)
		{
			fprintf(stderr, "%s: %s δ �� �� �� �� �� �� �� \n", SERVER_NAME,
				ERROR_MSG);
			return (void *)-1;
		}
		else
		{
			//inet_ntop(inc_addr.sin_family, inc_addr.sin_addr,clientaddr, sizeof(clientaddr));
			fprintf(stdout, "%s: %s ��⵽ %s ���ڳ������ӵ������� [socket ���: %d][�����û���:%d / %d]\n", SERVER_NAME, INFO_MSG, inet_ntoa(inc_addr.sin_addr), inc_fd, client_count(1), num_threads);
			if (((double)client_count(0) >= ((double)num_threads * TP_UTIL)) &&(client_count(0) <= num_threads))
			{
				if (client_count(0) == num_threads)
					fprintf(stdout, "%s: %s �� �� �� �� Դ �� �� [ �� �� �� ����: %d / %d]\n", SERVER_NAME, WARN_MSG, client_count(0), num_threads);
				else
					fprintf(stdout, "%s: %s �� �� �� �� Դ �� �� �� �� [ �� �� �� ����: %d / %d]\n", SERVER_NAME, WARN_MSG, client_count(0), num_threads);
			}
			else if ((client_count(0)) > num_threads)
			{
				fprintf(stderr, "%s: %s ���ӳ���Դ�ľ�����Ȼ�����û���������[�� �� �� �� �� : %d / %d]\n", SERVER_NAME, ERROR_MSG, client_count(0),num_threads);
				sprintf(out, "%s: %s �� �� �� �� �� �� �� , �� �� �� �� �� \n",SERVER_NAME, USER_MSG);
				send_msg(inc_fd, out);
			}
			params.fd = inc_fd;
			strcpy(params.ipaddr, inet_ntoa(inc_addr.sin_addr));
			thpool_add_work(thpool, &p2p, (void*)&params);//��ӵ��̳߳�
		}
	}
}