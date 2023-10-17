#include "Common.h"

// TCP Ŭ���̾�Ʈ ���� �κ�
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;
	int protocol;
	char message[BUFSIZE];
	int data1;
	int data2;
	int tempindex;
	int tempscore;

	// ���� ����
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// ������ ������ ���
	while (1)
	{
		bool endflag = false;

		if (!PacketRecv(sock, buf))
		{
			break;
		}

		PROTOCOL protocol = GetProtocol(buf);

		// �������ݿ� ���� �з�
		switch (protocol)
		{
			// ������ ���۵��� ���� ��� (���ڿ� ���)
			case PROTOCOL::WAIT:
			{
				memset(msg, 0, sizeof(msg));
				UnPackPacket(buf, msg);
				InvalidateRect(hWnd, NULL, FALSE);
			}
			break;

			// ������ ���۵� ���
			case PROTOCOL::START:
			{
				// �÷��̾� ����(index, score), ī�� ���� ����
				memset((char*)&arCell[0][0], 0, sizeof(tag_Cell) * 16);
				UnPackPacket(buf, index, tempscore, (char*)&arCell[0][0]);
				for (int i = 0; i < PLAYER_COUNT; ++i)
				{
					score[i] = tempscore;
				}
				InvalidateRect(hWnd, NULL, FALSE);

				InitGame();
			}
			break;

			// ���� ������ ���
			case PROTOCOL::SCORE:
			{
				UnPackPacket(buf, score[index - 1]);
				InvalidateRect(hWnd, NULL, FALSE);
			}
			break;

			// ¦�� �� ���� ���
			case PROTOCOL::OTHERSCORE:
			{
				UnPackPacket(buf, tempindex, tempscore);
				score[tempindex - 1] = tempscore;
				InvalidateRect(hWnd, NULL, FALSE);
			}
			break;

			// ¦�� ���� ���
			case PROTOCOL::SUCCESS:
			{
				UnPackPacket(buf, data1, data2, tempindex, tempscore);
				arCell[data1 / 4][data1 % 4].St = FLIP;
				arCell[data2 / 4][data2 % 4].St = FLIP;
				score[tempindex - 1] = tempscore;
				InvalidateRect(hWnd, NULL, FALSE);
			}
			break;

			// ¦�� �� ���� ���
			case PROTOCOL::FAILURE:
			{
				UnPackPacket(buf, data1, data2, tempindex, tempscore);
				arCell[data1 / 4][data1 % 4].St = TEMPFLIP;
				arCell[data2 / 4][data2 % 4].St = TEMPFLIP;
				score[tempindex - 1] = tempscore;
				GameStatus = _GameStatus::VIEW;
				SetTimer(hWnd, 1, 1000, NULL);
				InvalidateRect(hWnd, NULL, FALSE);
			}
			break;

			// ���� ���� ó��
			case PROTOCOL::GAME_ERROR:
			{
				int err_code;
				memset(msg, 0, sizeof(msg));
				UnPackPacket(buf, err_code, msg);
				InvalidateRect(hWnd, NULL, TRUE);
			}
			break;
			case PROTOCOL::GAME_RESULT:
			{
				memset(msg, 0, sizeof(msg));
				int result;
				UnPackPacket(buf, result, msg);
				GameStatus = _GameStatus::JOIN;
				InvalidateRect(hWnd, NULL, TRUE);
				endflag = true;
			}
			break;
			case PROTOCOL::INVALID_GAME:
			{
				memset(msg, 0, sizeof(msg));
				UnPackPacket(buf, msg);
				GameStatus = _GameStatus::JOIN;
				InvalidateRect(hWnd, NULL, TRUE);
				endflag = true;
			}
			break;
		}

		if (endflag)
		{
			break;
		}
	}

	closesocket(sock);
	sock = NULL;

	return 0;
}
