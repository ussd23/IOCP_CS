#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <winsock2.h> // 윈속2 메인 헤더
#include <ws2tcpip.h> // 윈속2 확장 헤더

#include <tchar.h> // _T(), ...
#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...

#pragma comment(lib, "ws2_32") // ws2_32.lib 링크

void err_quit(char* msg);
void err_display(char* msg);
void err_display(int errcode);

#include <time.h>
#include <tchar.h>
#include <queue>

#define SERVERPORT 9000
#define BUFSIZE    4096
#define GAME_OVER_NUMBER 8
#define PLAYER_COUNT 3	// 한 방에 참여할 플레이어 수

#define WAIT_MSG "상대를 기다리고 있습니다."
#define WIN_MSG "당신이 승리했습니다."
#define LOSE_MSG "당신이 패배했습니다."
#define DRAW_MSG "무승부입니다."
#define QUIT_MSG "상대가 게임을 나갔습니다."

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
	int				game_number;				// 게임의 번호
	GAME_STATE		state;						// 게임 상태
	_ClientInfo*	players[PLAYER_COUNT];		// 플레이어 클라이언트 정보
	int				player_count;				// 플레이어 수
	bool			full;						// 풀방 여부
	tag_Cell		arCell[4][4];				// 해당 게임의 카드 상태
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

// 송신 패킷
struct _SendPacket
{
	EXOVERLAPPED	s_overlapped;
	int				sendbytes;
	int				comp_sendbytes;
	char			sendbuf[BUFSIZE];
	WSABUF			s_wsabuf;
};

// 수신 패킷
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
	SOCKET			sock;			// 소켓
	SOCKADDR_IN		addr;			// 클라이언트 주소

	CLIENT_STATE	state;			// 클라이언트 상태
	int				player_number;	// 플레이어 index
	_GameInfo*		game_info;		// 게임 정보
	RESULT			game_result;	// 게임 결과 (승리/패배/무승부)
	int				score;			// 점수 (짝을 찾으면 +10, 못 찾으면 -1)

	_RecvPacket	RecvPacket;					// 수신 패킷
	std::queue<_SendPacket*>* SendQueue;	// 송신 패킷(Queue)
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
