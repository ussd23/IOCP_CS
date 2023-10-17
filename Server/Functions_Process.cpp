#include "Functions.h"

void RecvPacketProcess(_ClientInfo* _ptr)
{
	// ���������� Ȯ��
	PROTOCOL protocol = GetProtocol(_ptr->RecvPacket.recvbuf);
	_GameInfo* game = _ptr->game_info;
	char msg[BUFSIZE] = { 0 };
	char buf[BUFSIZE];
	int size;

	switch (_ptr->state)
	{
		// Init State������ ��Ŷ�� ���� ����.
		case CLIENT_STATE::C_INIT_STATE:
		break;

		// ������ �������� ���
		case CLIENT_STATE::C_GAME_STATE:
		{
			switch (protocol)
			{
				// �÷��̾ ������ ���
				case PROTOCOL::SELECT:
				{
					int data1, data2;
					UnPackPacket(_ptr->RecvPacket.recvbuf, data1, data2);

					// ¦�� ���� ���
					if (game->arCell[data1 / 4][data1 % 4].Num == game->arCell[data2 / 4][data2 % 4].Num)
					{
						EnterCriticalSection(&cs);
						// �ش� �÷��̾��� ���� ����, ���� ������ ����
						_ptr->score += 10;
						++game->game_number;

						game->arCell[data1 / 4][data1 % 4].St = FLIP;
						game->arCell[data2 / 4][data2 % 4].St = FLIP;
						LeaveCriticalSection(&cs);

						// ������ �����Ǿ��� �� üũ
						if (!GameContinueCheck(_ptr, game->game_number))
						{
							GameOverProcess(_ptr);
							return;
						}

						// Ŭ���̾�Ʈ���� ����� ī�� ���� �� ���� ���� ����
						for (int i = 0; i < game->player_count; ++i)
						{
							size = PackPacket(buf, PROTOCOL::SUCCESS, data1, data2, _ptr->player_number, _ptr->score);
							Send(game->players[i], buf, size);
						}
					}

					// ¦�� �� ���� ���
					else
					{
						EnterCriticalSection(&cs);
						--_ptr->score;
						LeaveCriticalSection(&cs);

						// ������ �����Ǿ��� �� üũ
						if (!GameContinueCheck(_ptr, game->game_number))
						{
							GameOverProcess(_ptr);
							return;
						}

						// Ŭ���̾�Ʈ���� ����� ���� ���� ����
						for (int i = 0; i < game->player_count; ++i)
						{
							if (_ptr == game->players[i])
							{
								size = PackPacket(buf, PROTOCOL::FAILURE, data1, data2, _ptr->player_number, _ptr->score);
								Send(game->players[i], buf, size);
							}
							else
							{
								size = PackPacket(buf, PROTOCOL::OTHERSCORE, _ptr->player_number, _ptr->score);
								Send(game->players[i], buf, size);
							}
						}
					}
				}
				break;
			}
		}
		break;
		case CLIENT_STATE::C_GAME_OVER_STATE:
		break;
		case CLIENT_STATE::C_DISCONNECTED_STATE:
		break;
	}
}

void SendPacketProcess(_ClientInfo* _ptr)
{
	_GameInfo* game_info = _ptr->game_info;
	char buf[BUFSIZE];
	int size;

	switch (_ptr->state)
	{
		// Init State�� ���
		case CLIENT_STATE::C_INIT_STATE:
		{
			switch (game_info->state)
			{
				// ������ ���۵� ��� Ŭ���̾�Ʈ�� State ����
				case GAME_STATE::G_PLAYING_STATE:
				{
					_ptr->state = CLIENT_STATE::C_GAME_STATE;
				}
				break;
			}
		}
		break;
		case CLIENT_STATE::C_GAME_STATE:
		{
			switch (game_info->state)
			{
				case GAME_STATE::G_PLAYING_STATE:
				break;
				// ������ ����� ��� Ŭ���̾�Ʈ�� State ����
				case GAME_STATE::G_GAME_OVER_STATE:
				{
					_ptr->state = CLIENT_STATE::C_GAME_OVER_STATE;
				}
				break;
				case GAME_STATE::G_INVALID_GAME_STATE:
				{
					_ptr->state = CLIENT_STATE::C_GAME_OVER_STATE;
				}
				break;
			}
		}
		break;
		case CLIENT_STATE::C_GAME_OVER_STATE:
		break;
		case CLIENT_STATE::C_DISCONNECTED_STATE:
		break;
	}
}

void InitProcess(_ClientInfo* _ptr)
{
	_GameInfo* game_info = AddGameInfo(_ptr);
	char msg[BUFSIZE];
	char buf[BUFSIZE];
	int size;

	switch (game_info->state)
	{
		// ������ ���۵��� ���� ��� ��� �޽��� ����
		case GAME_STATE::G_WAIT_STATE:
		{
			size = PackPacket(buf, PROTOCOL::WAIT, WAIT_MSG);
			Send(_ptr, buf, size);
		}
		break;

		// ������ ���۵� ��� ī�带 �����ϰ� ī�� ���� ����
		case GAME_STATE::G_PLAYING_STATE:
		{
			EnterCriticalSection(&cs);
			for (int i = 1; i <= 8; i++)
			{
				for (int j = 0; j < 2; j++)
				{
					int x, y;
					do
					{
						x = random(4);
						y = random(4);
					} while (game_info->arCell[x][y].Num != 0);
					game_info->arCell[x][y].Num = i;
				}
			}
			LeaveCriticalSection(&cs);

			for (int i = 0; i < game_info->player_count; ++i)
			{
				size = PackPacket(buf, PROTOCOL::START, game_info->players[i]->player_number, game_info->players[i]->score, &game_info->arCell[0][0]);
				Send(game_info->players[i], buf, size);
			}
		}
		break;
	}
}

void GameOverProcess(_ClientInfo* _ptr)
{
	char buf[BUFSIZE];
	char msg[BUFSIZE] = { 0 };
	int size;
	int retval;
	_GameInfo* game = _ptr->game_info;

	if (game->state == GAME_STATE::G_INVALID_GAME_STATE)
	{
		size = PackPacket(buf, PROTOCOL::INVALID_GAME, QUIT_MSG);
		Send(_ptr, buf, size);
		return;
	}

	// �� �÷��̾��� ��� ����
	for (int i = 0; i < game->player_count; ++i)
	{
		switch (game->players[i]->game_result)
		{
			case RESULT::WIN:
			{
				size = PackPacket(buf, PROTOCOL::GAME_RESULT, RESULT::WIN, WIN_MSG);
				Send(game->players[i], buf, size);
			}
			break;
			case RESULT::LOSE:
			{
				size = PackPacket(buf, PROTOCOL::GAME_RESULT, RESULT::LOSE, LOSE_MSG);
				Send(game->players[i], buf, size);
			}
			break;
			case RESULT::DRAW:
			{
				size = PackPacket(buf, PROTOCOL::GAME_RESULT, RESULT::DRAW, DRAW_MSG);
				Send(game->players[i], buf, size);
			}
			break;
		}
	}
}

void DisconnectedProcess(_ClientInfo* _ptr)
{
	// Ż���� ó��
	UpdateGameInfo(_ptr);

	// Ŭ���̾�Ʈ ���� ����
	RemoveClientInfo(_ptr);
}

bool GameContinueCheck(_ClientInfo* _ptr, int _game_number)
{
	EnterCriticalSection(&cs);
	_GameInfo* game = _ptr->game_info;

	// ������ 0�� �� �÷��̾ �ִ� �� Ȯ��
	int loseindex = -1;
	for (int i = 0; i < game->player_count; i++)
	{
		if (game->players[i]->score <= 0)
		{
			loseindex = i;
		}
	}

	// ������ 0�� �� �÷��̾ �ִ� ��� �ش� �÷��̾��� �й� ó��
	if (loseindex >= 0)
	{
		for (int i = 0; i < game->player_count; i++)
		{
			if (i == loseindex)
			{
				game->players[i]->game_result = RESULT::LOSE;
			}
			else
			{
				game->players[i]->game_result = RESULT::WIN;
			}
		}

		LeaveCriticalSection(&cs);
		return false;
	}

	// ��� ī�尡 �������� ���
	if (_game_number == GAME_OVER_NUMBER)
	{
		_ptr->game_result = RESULT::LOSE;
		game->state = GAME_STATE::G_GAME_OVER_STATE;

		int num = -1;
		int count = 0;
		
		// ���� ���� ������ ����ϰ� ������ ������ �÷��̾ �ִ� �� Ȯ��
		for (int i = 0; i < game->player_count; i++)
		{
			if (game->players[i]->score > num)
			{
				num = game->players[i]->score;
				count = 1;
			}
			else if (game->players[i]->score == num)
			{
				++count;
			}
		}

		// ���� ���� ������ ���� ���̰�, ��� �÷��̾��� ������ ���� ���� ��� (���� �¸� ����)
		if (count < game->player_count)
		{
			for (int i = 0; i < game->player_count; i++)
			{
				if (game->players[i]->score == num)
				{
					game->players[i]->game_result = RESULT::WIN;
				}
				else
				{
					game->players[i]->game_result = RESULT::LOSE;
				}
			}
		}

		// ��� �÷��̾��� ������ ���� ��� (���º� ó��)
		else if (count == game->player_count)
		{
			for (int i = 0; i < game->player_count; i++)
			{
				game->players[i]->game_result = RESULT::DRAW;
			}
		}
		LeaveCriticalSection(&cs);
		return false;
	}
	LeaveCriticalSection(&cs);

	return true;
}

void UpdateGameInfo(_ClientInfo* _ptr)
{
	EnterCriticalSection(&cs);
	_GameInfo* game = _ptr->game_info;

	// ��Ż�� �÷��̾�� ���� ���� �۾�
	for (int i = 0; i < game->player_count; i++)
	{
		if (game->players[i] == _ptr)
		{
			for (int j = i; j < game->player_count - 1; j++)
			{
				game->players[j] = game->players[j + 1];
			}
			game->player_count--;

			// �� �� ���� ��� ���� ���� ó��
			if (game->state == GAME_STATE::G_PLAYING_STATE &&
				game->player_count == 1)
			{
				game->state = GAME_STATE::G_INVALID_GAME_STATE;
				GameOverProcess(game->players[0]);
				break;
			}

			if (game->player_count == 0)
			{
				ReMoveGameInfo(game);
			}
			break;
		}

	}
	LeaveCriticalSection(&cs);
}