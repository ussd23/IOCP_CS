#define _CRT_SECURE_NO_WARNINGS // ���� C �Լ� ��� �� ��� ����
#define _WINSOCK_DEPRECATED_NO_WARNINGS // ���� ���� API ��� �� ��� ����

#include <winsock2.h> // ����2 ���� ���
#include <ws2tcpip.h> // ����2 Ȯ�� ���

#include <tchar.h> // _T(), ...
#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...

#pragma comment(lib, "ws2_32") // ws2_32.lib ��ũ

void err_quit(char* msg);
void err_display(char* msg);
void err_display(int errcode);

#include <time.h>
#include <tchar.h>
#include <queue>

#define SERVERPORT 9000
#define BUFSIZE    4096
#define GAME_OVER_NUMBER 8
#define PLAYER_COUNT 3	// �� �濡 ������ �÷��̾� ��

#define WAIT_MSG "��븦 ��ٸ��� �ֽ��ϴ�."
#define WIN_MSG "����� �¸��߽��ϴ�."
#define LOSE_MSG "����� �й��߽��ϴ�."
#define DRAW_MSG "���º��Դϴ�."
#define QUIT_MSG "��밡 ������ �������ϴ�."

enum CLIENT_STATE
{
	C_INIT_STATE = 1,
	C_GAME_STATE,
	C_GAME_OVER_STATE,
	C_DISCONNECTED_STATE
};

enum PROTOCOL
{
	WAIT = 1,
	START,
	SELECT,
	SCORE,
	OTHERSCORE,
	SUCCESS,
	FAILURE,
	TEMP,
	INVALID_GAME,
	GAME_ERROR,
	GAME_RESULT
};

enum RESULT
{
	NODATA = -1,
	INPUT_DATA = 1,
	DATA_ERROR,
	WIN,
	LOSE,
	DRAW,
	EXIT
};

enum ERROR_CODE
{
	SELECT_COUNT_ERROR = 1
};

struct _ClientInfo;

enum GAME_STATE
{
	G_WAIT_STATE = 1,
	G_PLAYING_STATE,
	G_GAME_OVER_STATE,
	G_INVALID_GAME_STATE
};

enum Status { HIDDEN, FLIP, TEMPFLIP };
struct tag_Cell
{
	int Num;
	Status St;
};

struct _GameInfo
{
	int				game_number;				// ������ ��ȣ
	GAME_STATE		state;						// ���� ����
	_ClientInfo*	players[PLAYER_COUNT];		// �÷��̾� Ŭ���̾�Ʈ ����
	int				player_count;				// �÷��̾� ��
	bool			full;						// Ǯ�� ����
	tag_Cell		arCell[4][4];				// �ش� ������ ī�� ����
};

enum IO_TYPE
{
	IO_RECV,
	IO_SEND
};

enum SOC_RESULT
{
	SOC_ERROR = 1,
	SOC_TRUE,
	SOC_FALSE
};

struct EXOVERLAPPED
{
	WSAOVERLAPPED	overlapped;
	_ClientInfo*	ptr;
	IO_TYPE			iotype;
};

// �۽� ��Ŷ
struct _SendPacket
{
	EXOVERLAPPED	s_overlapped;
	int				sendbytes;
	int				comp_sendbytes;
	char			sendbuf[BUFSIZE];
	WSABUF			s_wsabuf;
};

// ���� ��Ŷ
struct _RecvPacket
{
	EXOVERLAPPED	r_overlapped;
	bool			r_sizeflag;
	int				recvbytes;
	int				comp_recvbytes;
	char			recvbuf[BUFSIZE];
	WSABUF			r_wsabuf;
};

struct _ClientInfo
{
	SOCKET			sock;			// ����
	SOCKADDR_IN		addr;			// Ŭ���̾�Ʈ �ּ�

	CLIENT_STATE	state;			// Ŭ���̾�Ʈ ����
	int				player_number;	// �÷��̾� index
	_GameInfo*		game_info;		// ���� ����
	RESULT			game_result;	// ���� ��� (�¸�/�й�/���º�)
	int				score;			// ���� (¦�� ã���� +10, �� ã���� -1)

	_RecvPacket	RecvPacket;					// ���� ��Ŷ
	std::queue<_SendPacket*>* SendQueue;	// �۽� ��Ŷ(Queue)
};

#ifdef MAIN
_ClientInfo* ClientInfo[100];
int Count = 0;

_GameInfo* GameInfo[100];
int GameCount = 0;

CRITICAL_SECTION cs;

#else

extern _ClientInfo* ClientInfo[100];
extern int Count;

extern _GameInfo* GameInfo[100];
extern int GameCount;

extern CRITICAL_SECTION cs;
#endif
