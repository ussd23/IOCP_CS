#define MAIN
#include "Functions.h"

// 비동기 입출력 처리 함수
DWORD WINAPI WorkerThread(LPVOID arg);

int main(int argc, char* argv[])
{
	srand(time(NULL));
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	InitializeCriticalSection(&cs);

	// 입출력 완료 포트 생성
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) return 1;

	// CPU 개수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// (CPU 개수 * 2)개의 작업자 스레드 생성
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	{
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
		if (hThread == NULL) return 1;
		CloseHandle(hThread);
	}

	// 소켓 생성
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

	// 데이터 통신에 사용할 변수
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

		// 소켓과 입출력 완료 포트 연결
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		_ClientInfo* ptr = AddClientInfo(client_sock, clientaddr);
		if (ptr == nullptr)
		{
			continue;
		}

		// 최초의 Recv
		if (!Recv(ptr))
		{
			RemoveClientInfo(ptr);
		}
	}

	DeleteCriticalSection(&cs);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 작업자 스레드 함수
DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;

	while (1)
	{
		// 비동기 입출력 완료 기다리기
		DWORD cbTransferred;
		SOCKET client_sock;
		EXOVERLAPPED* overlapped;

		// PostQueuedCompletionStatus(hcp, -100, -100, NULL);		// 프로그램에서 메시지 제어
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred,
			(PULONG_PTR)&client_sock, (LPOVERLAPPED*)&overlapped, INFINITE);

		// 클라이언트 정보(IP 주소, 포트 번호) 얻기
		_ClientInfo* ptr = overlapped->ptr;
		IO_TYPE iotype = overlapped->iotype;

		// 비동기 입출력 결과 확인
		if (retval == 0 || cbTransferred == 0)
		{
			// 비정상적인 결과 확인
			DWORD transferred, flags;
			int result = WSAGetOverlappedResult(ptr->sock, &overlapped->overlapped, &transferred, FALSE, &flags);

			if (result == 0)
			{
				err_display("Completion Error");
			}

			DisconnectedProcess(ptr);
			continue;
		}

		// 입력/출력 구분
		switch (iotype)
		{
		case IO_TYPE::IO_RECV:

			// 모두 수신이 완료되었는 지 확인
			retval = CompleteRecv(ptr, cbTransferred);
			switch (retval)
			{
			case SOC_ERROR:
			case SOC_FALSE:
				continue;
			case SOC_TRUE:
				break;
			}

			// 수신 완료 시 처리
			RecvPacketProcess(ptr);

			// 이후 Recv
			if (!Recv(ptr))
			{
				DisconnectedProcess(ptr);
				continue;
			}

			break;
		case IO_TYPE::IO_SEND:

			// 모두 송신이 완료되었는 지 확인
			retval = CompleteSend(ptr, cbTransferred);
			switch (retval)
			{
			case SOC_ERROR:
			case SOC_FALSE:
				continue;
			case SOC_TRUE:
				break;
			}

			// 송신 완료 시 처리
			SendPacketProcess(ptr);
			break;
		}
	}

	return 0;
}