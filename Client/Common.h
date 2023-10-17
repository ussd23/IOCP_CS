#pragma once
#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <winsock2.h> // 윈속2 메인 헤더
#include <ws2tcpip.h> // 윈속2 확장 헤더
#include <windows.h>

#include <tchar.h> // _T(), ...
#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...
#include "resource.h"

#pragma comment(lib, "ws2_32")

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512
#define PLAYER_COUNT 3

enum _GameStatus { JOIN, RUN, HINT, VIEW };

enum Status { HIDDEN, FLIP, TEMPFLIP };
struct tag_Cell
{
	int Num;
	Status St;
};

#ifdef MAIN
WSADATA wsa;
SOCKET sock;

HWND hWnd;
tag_Cell arCell[4][4];
int count;
HBITMAP hShape[9];
_GameStatus GameStatus;
int index;
int score[PLAYER_COUNT] = { 0, };
char buf[BUFSIZE];
char msg[BUFSIZE] = "연결 시도중...";
#else
extern WSADATA wsa;
extern SOCKET sock;

extern HWND hWnd;
extern tag_Cell arCell[4][4];
extern int count;
extern HBITMAP hShape[9];
extern _GameStatus GameStatus;
extern int index;
extern int score[PLAYER_COUNT];
extern char buf[BUFSIZE];
extern char msg[BUFSIZE];
#endif

void InitGame();
void DrawScreen(HDC hdc);
void GetTempFlip(int* tx, int* ty);
int GetRemain();
void DrawBitmap(HDC hdc, int x, int y, HBITMAP hBit);

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

int recvn(SOCKET s, char* buf, int len, int flags);
void err_quit(char* msg);
void err_display(char* msg);

bool PacketRecv(SOCKET _sock, char* _buf);
PROTOCOL GetProtocol(char* _buf);
int PackPacket(char*, PROTOCOL);
int PackPacket(char* _buf, PROTOCOL _protocol, int _data1, int _data2);
void UnPackPacket(char* _buf, int& _data1, int& _data2, int& _index, int& _score);
void UnPackPacket(char* _buf, char* _str1);
void UnPackPacket(char* _buf, int& _data, char* _str1);
void UnPackPacket(char* _buf, int& _data1, int& _data2, char* _str1);
void UnPackPacket(char* _buf, int& _data);
void UnPackPacket(char* _buf, int& _data1, int& _data2);

DWORD WINAPI ClientMain(LPVOID arg);