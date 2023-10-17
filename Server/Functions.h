#pragma once
#include "Common.h"

#define random(n) (rand()%n)

void RecvPacketProcess(_ClientInfo* _ptr);
void SendPacketProcess(_ClientInfo* _ptr);

bool Recv(_ClientInfo* _ptr);
bool Send(_ClientInfo* _ptr, char* _buf, int _size);
int CompleteRecv(_ClientInfo* _ptr, int cbTransferred);
int CompleteSend(_ClientInfo* _ptr, int cbTransferred);

PROTOCOL GetProtocol(const char* _ptr);
int PackPacket(char*, PROTOCOL, const char*);
int PackPacket(char*, PROTOCOL, int, int, tag_Cell*);
int PackPacket(char*, PROTOCOL, int, const char*);
int PackPacket(char*, PROTOCOL, int);
int PackPacket(char*, PROTOCOL, int, int);
int PackPacket(char*, PROTOCOL, int, int, int, int);
void UnPackPacket(char*, int&, int&);

_ClientInfo* AddClientInfo(SOCKET _sock, SOCKADDR_IN _clientaddr);
void RemoveClientInfo(_ClientInfo* _ptr);
_GameInfo* AddGameInfo(_ClientInfo*);
void ReMoveGameInfo(_GameInfo*);

void InitProcess(_ClientInfo*);
void GameOverProcess(_ClientInfo*);
void DisconnectedProcess(_ClientInfo*);
bool GameContinueCheck(_ClientInfo*, int);
void UpdateGameInfo(_ClientInfo* _ptr);