#include "Functions.h"

// Ŭ���̾�Ʈ ���� �߰�
_ClientInfo* AddClientInfo(SOCKET _sock, SOCKADDR_IN _clientaddr)
{
	EnterCriticalSection(&cs);

	if (Count >= 100)
	{
		printf("�� �̻� ������ ������ �� �����ϴ�.");
		LeaveCriticalSection(&cs);
		return nullptr;
	}

	//���� ����ü �迭�� ���ο� ���� ���� ����ü ����
	_ClientInfo* ptr = new _ClientInfo;
	ZeroMemory(ptr, sizeof(_ClientInfo));

	ptr->sock = _sock;
	memcpy(&ptr->addr, &_clientaddr, sizeof(SOCKADDR_IN));
	ptr->RecvPacket.r_sizeflag = false;
	ptr->RecvPacket.recvbytes = sizeof(int);
	ptr->state = CLIENT_STATE::C_INIT_STATE;
	ptr->player_number = NODATA;
	ptr->game_result = NODATA;

	ptr->SendQueue = new std::queue<_SendPacket*>;

	ptr->RecvPacket.r_overlapped.ptr = ptr;
	ptr->RecvPacket.r_overlapped.iotype = IO_TYPE::IO_RECV;

	ClientInfo[Count++] = ptr;

	LeaveCriticalSection(&cs);

	printf("\nClient ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", inet_ntoa(_clientaddr.sin_addr),
		ntohs(_clientaddr.sin_port));

	InitProcess(ptr);

	return ptr;
}

// Ŭ���̾�Ʈ ���� ����
void RemoveClientInfo(_ClientInfo* _ptr)
{
	printf("\nClient ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", inet_ntoa(_ptr->addr.sin_addr), ntohs(_ptr->addr.sin_port));

	EnterCriticalSection(&cs);

	closesocket(_ptr->sock);

	for (int i = 0; i < Count; i++)
	{
		if (ClientInfo[i] == _ptr)
		{
			delete ClientInfo[i]->SendQueue;
			delete ClientInfo[i];
			for (int j = i; j < Count - 1; j++)
			{
				ClientInfo[j] = ClientInfo[j + 1];
			}
			ClientInfo[Count - 1] = nullptr;
			Count--;
			break;
		}
	}

	LeaveCriticalSection(&cs);
}

// ���� ���� �߰�
_GameInfo* AddGameInfo(_ClientInfo* _ptr)
{
	EnterCriticalSection(&cs);

	_GameInfo* game_ptr = nullptr;
	int index = NODATA;

	// ������ �� �ִ� ���� �ִ� �� Ȯ��
	for (int i = 0; i < GameCount; i++)
	{
		if (!GameInfo[i]->full)
		{
			GameInfo[i]->players[GameInfo[i]->player_count++] = _ptr;
			_ptr->game_info = GameInfo[i];
			_ptr->player_number = GameInfo[i]->player_count;

			if (GameInfo[i]->player_count == PLAYER_COUNT)
			{
				GameInfo[i]->full = true;
				GameInfo[i]->state = GAME_STATE::G_PLAYING_STATE;
			}
			game_ptr = GameInfo[i];
			index = i;
			break;
		}
	}

	// ������ �� �ִ� ���� ���� ���
	if (index == NODATA)
	{
		game_ptr = new _GameInfo;
		ZeroMemory(game_ptr, sizeof(_GameInfo));
		game_ptr->full = false;
		game_ptr->players[0] = _ptr;

		_ptr->game_info = game_ptr;
		_ptr->player_number = 1;

		game_ptr->player_count++;
		game_ptr->state = GAME_STATE::G_WAIT_STATE;
		GameInfo[GameCount++] = game_ptr;
	}
	_ptr->score = 10;

	LeaveCriticalSection(&cs);
	return game_ptr;
}

// ���� ���� ����
void ReMoveGameInfo(_GameInfo* _ptr)
{
	EnterCriticalSection(&cs);
	for (int i = 0; i < GameCount; i++)
	{
		if (GameInfo[i] == _ptr)
		{
			delete _ptr;
			for (int j = i; j < GameCount - 1; j++)
			{
				GameInfo[j] = GameInfo[j + 1];
			}
			break;
		}
	}

	GameCount--;
	LeaveCriticalSection(&cs);
}
