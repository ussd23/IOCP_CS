#define MAIN
#include "Functions.h"

// �񵿱� ����� ó�� �Լ�
DWORD WINAPI WorkerThread(LPVOID arg);

int main(int argc, char* argv[])
{
	srand(time(NULL));
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	InitializeCriticalSection(&cs);

	// ����� �Ϸ� ��Ʈ ����
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) return 1;

	// CPU ���� Ȯ��
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// (CPU ���� * 2)���� �۾��� ������ ����
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	{
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
		if (hThread == NULL) return 1;
		CloseHandle(hThread);
	}

	// ���� ����
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	DWORD recvbytes, flags;

	while (1)
	{
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accept()");
			continue;
		}

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		_ClientInfo* ptr = AddClientInfo(client_sock, clientaddr);
		if (ptr == nullptr)
		{
			continue;
		}

		// ������ Recv
		if (!Recv(ptr))
		{
			RemoveClientInfo(ptr);
		}
	}

	DeleteCriticalSection(&cs);

	// ���� ����
	WSACleanup();
	return 0;
}

// �۾��� ������ �Լ�
DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1)
	{
		// �񵿱� ����� �Ϸ� ��ٸ���
		DWORD cbTransferred;
		SOCKET client_sock;
		EXOVERLAPPED* overlapped;

		// PostQueuedCompletionStatus(hcp, -100, -100, NULL);		// ���α׷����� �޽��� ����
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred,
			(PULONG_PTR)&client_sock, (LPOVERLAPPED*)&overlapped, INFINITE);

		// Ŭ���̾�Ʈ ����(IP �ּ�, ��Ʈ ��ȣ) ���
		_ClientInfo* ptr = overlapped->ptr;
		IO_TYPE iotype = overlapped->iotype;

		// �񵿱� ����� ��� Ȯ��
		if (retval == 0 || cbTransferred == 0)
		{
			// ���������� ��� Ȯ��
			DWORD transferred, flags;
			int result = WSAGetOverlappedResult(ptr->sock, &overlapped->overlapped, &transferred, FALSE, &flags);

			if (result == 0)
			{
				err_display("Completion Error");
			}

			DisconnectedProcess(ptr);
			continue;
		}

		// �Է�/��� ����
		switch (iotype)
		{
		case IO_TYPE::IO_RECV:

			// ��� ������ �Ϸ�Ǿ��� �� Ȯ��
			retval = CompleteRecv(ptr, cbTransferred);
			switch (retval)
			{
			case SOC_ERROR:
			case SOC_FALSE:
				continue;
			case SOC_TRUE:
				break;
			}

			// ���� �Ϸ� �� ó��
			RecvPacketProcess(ptr);

			// ���� Recv
			if (!Recv(ptr))
			{
				DisconnectedProcess(ptr);
				continue;
			}

			break;
		case IO_TYPE::IO_SEND:

			// ��� �۽��� �Ϸ�Ǿ��� �� Ȯ��
			retval = CompleteSend(ptr, cbTransferred);
			switch (retval)
			{
			case SOC_ERROR:
			case SOC_FALSE:
				continue;
			case SOC_TRUE:
				break;
			}

			// �۽� �Ϸ� �� ó��
			SendPacketProcess(ptr);
			break;
		}
	}

	return 0;
}