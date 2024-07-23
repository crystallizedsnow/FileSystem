#include "Winsock.h"
#include "windows.h"
#include <iostream>
#include <string>
#include <vector>
#define TERMINATION_MARKER "###END_OF_LIST###\n"
using namespace std;

#define RECV_PORT 3000	//���ն˿�
#define SEND_PORT 4000	//���Ͷ˿�
#pragma comment(lib, "wsock32.lib")


SOCKET sockClient;//�ͻ����׽���
sockaddr_in severAddr;//��������ַ
sockaddr_in ClientAddr;//�ͻ��˵�ַ 
vector<HANDLE> hThread;//�߳�vector
int tnum = 0;//�߳���Ŀ

int addrLen;		//��ַ����
char fileName[20];	//�ļ���
char order[20];		//����


char namePassword[1024] = "c 123";	//�û��������루�����û���������¼��



DWORD startSock();
DWORD createSocket();
int sendFileRecord(SOCKET datatcps, WIN32_FIND_DATA* pfd);
int sendFileList(SOCKET datatcps);
int sendFile(SOCKET& datatcps, FILE* file,char*sbuff);
DWORD connectProcess();
DWORD WINAPI ThreadFun(LPVOID lpThreadParameter);

DWORD startSock() {//��ʼ��winsock
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		cout << "��ʼ��ʧ��" << endl;
		return -1;
	}
	return 1;
}
DWORD createSocket() {
	sockClient = socket(AF_INET, SOCK_STREAM, 0);		//sockclient�Ǵ��ڼ������׽���
	if (sockClient == SOCKET_ERROR) {
		cout << "����ʧ��" << endl;
		WSACleanup();
		return -1;
	}
	severAddr.sin_family = AF_INET;	//ClientAddr �Ǽ����Ķ����ַ
	severAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	severAddr.sin_port = htons(RECV_PORT);
	if (bind(sockClient, (struct sockaddr FAR*) & severAddr, sizeof(severAddr)) == SOCKET_ERROR)		//bind�������ڽ�socket�͵�ַ�ṹ��
	{
		cout << "��ʧ��" << endl;
		return -1;
	}
	return 1;
}


DWORD connectProcess() 
{
	addrLen = sizeof(ClientAddr);//addrLen�Ƕ����ַ�ĳ��� 
	if (listen(sockClient, 10) < 0) {//���׽��ֽ��뱻������״̬������2Ϊ������е���󳤶�
		cout << "����ʧ��" << endl;
		return -1;
	}
	cout << "���������ڼ����С�" << endl;
	SOCKET sockServer;
	while (1) 
	{
		sockServer = accept(sockClient, (struct sockaddr FAR*) & ClientAddr, &addrLen);//acceptȡ������ͷ������������
		if (sockServer != INVALID_SOCKET)
		{
			hThread.push_back(CreateThread(NULL, 0, ThreadFun, (LPVOID)sockServer, 0, NULL));
			tnum++;
		}
	}
	/*for (int i = 0; i < hThread.size(); i++)
	{
		WaitForSingleObject(hThread[i], INFINITE);
		CloseHandle(hThread[i]);
	}*///�����ֶ��ͷ��߳�
}

int sendFile(SOCKET &datatcps, FILE* file,char*sbuff) 
{
	cout << "���ڷ����ļ���" << endl;
	memset(sbuff, '\0', sizeof(sbuff));
	while (1) {//���ļ���ѭ����ȡ���ݲ��������ͻ���
		int len = fread(sbuff, 1, sizeof(sbuff), file);//��fileָ��ָ����ļ��е����ݶ�ȡ��sbuff��
		if (send(datatcps, sbuff, len, 0) == SOCKET_ERROR) {
			cout << "����ʧ��" << endl;
			return 0;
		}
		if (len < sizeof(sbuff)) {//�ļ����ͽ���
			break;
		}
	}
	cout << "���ͳɹ�" << endl;
	return 1;
}
DWORD WINAPI ThreadFun(LPVOID lpThreadParameter)
{
	char rbuff[1024];	//���ջ�����
	char sbuff[1024];	//���ͻ�����
	SOCKET sockServer = (SOCKET)lpThreadParameter;
	cout << "�ɹ���" << sockServer << "�������ӣ�" << endl;
	while (1)
	{
		memset(rbuff, '\0', sizeof(rbuff));
		memset(sbuff, '\0', sizeof(sbuff));
		if (recv(sockServer, rbuff, sizeof(rbuff), 0) < 0) { break; }
		cout << endl << "��ȡ��ִ�е����" << rbuff << endl;
		if (strncmp(rbuff, "get", 3) == 0) {
			strcpy(fileName, rbuff + 4);
			FILE* file;//����һ���ļ�����ָ��
			file = fopen(fileName, "rb");//�����ƴ��ļ���ֻ�����  			//���������ļ�����
			if (file) {
				sprintf(sbuff, "get %s", fileName);
				if (!send(sockServer, sbuff, sizeof(sbuff), 0)) {
					fclose(file);
					return 0;
				}
				else {//���������������Ӵ�������
					if (!sendFile(sockServer, file, sbuff)) {
						return 0;
					}
					fclose(file);
				}
			}
			else {
				strcpy(sbuff, "�޷����ļ�\n");
				if (send(sockServer, sbuff, sizeof(sbuff), 0)) {
					return 0;
				}
			}
		}//get
		else if (strncmp(rbuff, "put", 3) == 0) {
			FILE* fd;
			int cnt;
			strcpy(fileName, rbuff + 4);
			fd = fopen(fileName, "wb");
			if (fd == NULL) {
				cout << "�޷����ļ�" << fileName << endl;
				return 0;
			}
			sprintf(sbuff, "put %s", fileName);
			if (!send(sockServer, sbuff, sizeof(sbuff), 0)) {
				fclose(fd);
				return 0;
			}
			memset(rbuff, '\0', sizeof(rbuff));
			while ((cnt = recv(sockServer, rbuff, sizeof(rbuff), 0)) > 0) {
				fwrite(rbuff, sizeof(char), cnt, fd);//��cnt�����ݳ���Ϊchar�����ݴ�rbuff���뵽fdָ����ļ�
				if (cnt < sizeof(rbuff))
				{
					break;
				}
			}
			cout << "�ɹ�����ļ�" << fileName << endl;
			fclose(fd);
		}//put
		else if (strncmp(rbuff, "pwd", 3) == 0) {
			char path[1000];
			GetCurrentDirectory(sizeof(path), path);//�ҵ���ǰ���̵ĵ�ǰĿ¼
			strcpy(sbuff, path);
			send(sockServer, sbuff, sizeof(sbuff), 0);
		}//pwd
		else if (strncmp(rbuff, "dir", 3) == 0) {
			strcpy(sbuff, rbuff);
			send(sockServer, sbuff, sizeof(sbuff), 0);
			sendFileList(sockServer);
		}//dir
		else if (strncmp(rbuff, "cd", 2) == 0) {
			strcpy(fileName, rbuff + 3);
			strcpy(sbuff, rbuff);
			send(sockServer, sbuff, sizeof(sbuff), 0);
			SetCurrentDirectory(fileName);//���õ�ǰĿ¼ 
		}//cd
		else if (strncmp(rbuff, "user", 4) == 0) {
			char tbuff[1024];
			strcpy(tbuff, rbuff + 5);
			strcat(tbuff, " ");
			memset(rbuff, '\0', sizeof(rbuff));
			strcpy(sbuff, "�ɹ���ȡ�û���\0");
			send(sockServer, sbuff, sizeof(sbuff), 0);

			recv(sockServer, rbuff, sizeof(rbuff), 0);
			cout << endl << "��ȡ��ִ�е����" << rbuff << endl;
			strcat(tbuff, rbuff + 5);
			if (strcmp(tbuff, namePassword) == 0) {//��֤�Ƿ���ȷ���������ݸ��ͻ���
				send(sockServer, "right\0", sizeof(sbuff), 0);
			}
			else {
				send(sockServer, "wrong\0", sizeof(sbuff), 0);
			}
		}//user pass
		else if (strncmp(rbuff, "changeID", 8) == 0)
		{
			memset(sbuff, '\0', sizeof(sbuff));
			strcpy(sbuff, "changeID");
			send(sockServer, sbuff, sizeof(sbuff), 0);
			memset(sbuff, '\0', sizeof(sbuff));
			memset(rbuff, '\0', sizeof(rbuff));
			char namePass[1024] = "\0";
			recv(sockServer, rbuff, sizeof(rbuff), 0);
			strcpy(namePass, rbuff);
			cout << "���ĺ���û�����������ϣ�" << namePass << endl;
			strcpy(namePassword, namePass);
			strcpy(sbuff, namePass);
			send(sockServer, sbuff, sizeof(sbuff), 0);
		}//changeID
	}
	cout << "��" << sockServer << "�Ͽ����ӣ�" << endl;
	tnum--;
	closesocket(sockServer);
}
int sendFileList(SOCKET datatcps) {
	HANDLE hff;								//����һ���߳�
	WIN32_FIND_DATA fd;						//�����ļ�
	hff = FindFirstFile("*", &fd);			//�����ļ����Ѵ������ļ���������Զ�ȡ��WIN32_FIND_DATA�ṹ��ȥ 
	if (hff == INVALID_HANDLE_VALUE) {		//��������
		const char* errStr = "�г��ļ��б�ʱ��������\n";
		cout << *errStr << endl;
		if (send(datatcps, errStr, strlen(errStr), 0) == SOCKET_ERROR) {
			cout << "����ʧ��" << endl;
		}
		return 0;
	}
	BOOL flag = TRUE;
	while (flag) {//�����ļ���Ϣ
		if (!sendFileRecord(datatcps, &fd)) {
			return 0;
		}
		flag = FindNextFile(hff, &fd);//������һ���ļ�
	}
	send(datatcps, TERMINATION_MARKER, strlen(TERMINATION_MARKER), 0);
	return 1;
}
int sendFileRecord(SOCKET datatcps, WIN32_FIND_DATA* pfd) {//���͵�ǰ���ļ���¼
	char fileRecord[MAX_PATH + 32];

	FILETIME ft;						//�ļ��Ľ���ʱ��
	FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ft);//Converts a file time to a local file time.

	SYSTEMTIME lastWriteTime;
	FileTimeToSystemTime(&ft, &lastWriteTime);

	const char* dir = pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "<DIR>" : " ";
	sprintf(fileRecord, "%04d-%02d-%02d %02d:%02d %5s %10d   %-20s\n",
		lastWriteTime.wYear,
		lastWriteTime.wMonth,
		lastWriteTime.wDay,
		lastWriteTime.wHour,
		lastWriteTime.wMinute,
		dir,
		pfd->nFileSizeLow,
		pfd->cFileName
	);
	if (send(datatcps, fileRecord, strlen(fileRecord), 0) == SOCKET_ERROR)		//ͨ��datatcps�ӿڷ���fileRecord���ݣ��ɹ����ط��͵��ֽ���  
	{
		cout << "����ʧ��" << endl;
		return 0;
	}
	return 1;
}
int main()
{
	if (startSock() == -1 || createSocket() == -1 || connectProcess() == -1)
	{
		return -1;
	}
	return 1;
}

