#define MAIN
#include "Common.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
HWND hWndMain;
LPCTSTR lpszClass = TEXT("Couple2");

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance
	  ,LPSTR lpszCmdParam,int nCmdShow)
{
	MSG Message;
	WNDCLASS WndClass;
	g_hInst=hInstance;
	
	WndClass.cbClsExtra=0;
	WndClass.cbWndExtra=0;
	WndClass.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
	WndClass.hCursor=LoadCursor(NULL,IDC_ARROW);
	WndClass.hIcon=LoadIcon(g_hInst,MAKEINTRESOURCE(IDI_COUPLE));
	WndClass.hInstance=hInstance;
	WndClass.lpfnWndProc=WndProc;
	WndClass.lpszClassName=lpszClass;
	WndClass.lpszMenuName=NULL;
	WndClass.style=0;
	RegisterClass(&WndClass);

	hWnd=CreateWindow(lpszClass,lpszClass, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT,CW_USEDEFAULT,0,0,NULL,(HMENU)NULL,hInstance,NULL);
	ShowWindow(hWnd, nCmdShow);
	
	// 윈속 초기화
	
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);
	
	while (GetMessage(&Message,NULL,0,0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	int nx,ny;
	int i,j;
	int tx,ty;
	RECT crt;
	int size;

	switch (iMessage) {
	case WM_CREATE:
		SetRect(&crt,0,0,64*4+250,64*4);
		AdjustWindowRect(&crt,WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,FALSE);
		SetWindowPos(hWnd,NULL,0,0,crt.right-crt.left,crt.bottom-crt.top,
			SWP_NOMOVE | SWP_NOZORDER);
		hWndMain=hWnd;
		for (i=0;i<sizeof(hShape)/sizeof(hShape[0]);i++) {
			hShape[i]=LoadBitmap(g_hInst,MAKEINTRESOURCE(IDB_SHAPE1+i));
		}
		srand(GetTickCount());
		GameStatus = _GameStatus::JOIN;
		return 0;
	case WM_LBUTTONDOWN:
		nx=LOWORD(lParam)/64;
		ny=HIWORD(lParam)/64;
		if (GameStatus != RUN || nx > 3 || ny > 3 || arCell[nx][ny].St != HIDDEN) {
			return 0;
		}

		GetTempFlip(&tx, &ty);
		if (tx == -1)
		{
			arCell[nx][ny].St = TEMPFLIP;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		else
		{
			count++;

			// 데이터 전송
			size = PackPacket(buf, PROTOCOL::SELECT, nx * 4 + ny, tx * 4 + ty);
			send(sock, buf, size, 0);
			
		}
		return 0;
	case WM_TIMER:
		switch (wParam) {
		case 0:
			KillTimer(hWnd,0);
			GameStatus= _GameStatus::RUN;
			InvalidateRect(hWnd,NULL,FALSE);
			break;
		case 1:
			KillTimer(hWnd,1);
			if (GameStatus == _GameStatus::JOIN) break;
			GameStatus= _GameStatus::RUN;
			for (i=0;i<4;i++) {
				for (j=0;j<4;j++) {
					if (arCell[i][j].St==TEMPFLIP) 
						arCell[i][j].St=HIDDEN;
				}
			}
			InvalidateRect(hWnd,NULL,FALSE);
			break;
		}
		return 0;
	case WM_PAINT:
		hdc=BeginPaint(hWnd, &ps);
		DrawScreen(hdc);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		for (i=0;i<sizeof(hShape)/sizeof(hShape[0]);i++) {
			DeleteObject(hShape[i]);
		}
		// closesocket()
		if (sock != NULL) closesocket(sock);

		// 윈속 종료
		WSACleanup();
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd,iMessage,wParam,lParam));
}

void InitGame()
{
	count=0;

	GameStatus= _GameStatus::HINT;
	InvalidateRect(hWndMain,NULL,TRUE);
	SetTimer(hWndMain,0,2000,NULL);
}

void DrawScreen(HDC hdc)
{
	int x,y,image;
	TCHAR Mes[128];

	for (x=0;x<4;x++) {
		for (y=0;y<4;y++) {
			if (GameStatus==HINT || arCell[x][y].St!=HIDDEN) {
				image=arCell[x][y].Num-1;
			} else {
				image=8;
			}
			DrawBitmap(hdc,x*64,y*64,hShape[image]);
		}
	}

	lstrcpy(Mes,"짝 맞추기 게임 Ver. Online");
	TextOut(hdc,300,10,Mes,lstrlen(Mes));

	// 게임이 시작되지 않은 경우
	if (GameStatus == _GameStatus::JOIN)
	{
		wsprintf(Mes, "%s", msg);
		TextOut(hdc, 300, 50, Mes, lstrlen(Mes));
	}
	
	// 게임이 시작된 경우 화면상에 각종 정보 표시
	else
	{
		wsprintf(Mes, "총 시도 회수 : %d        ", count);
		TextOut(hdc, 300, 50, Mes, lstrlen(Mes));
		wsprintf(Mes, "아직 못 찾은 것 : %d        ", GetRemain());
		TextOut(hdc, 300, 70, Mes, lstrlen(Mes));
		wsprintf(Mes, "내 점수 : %d        ", score[index - 1]);
		TextOut(hdc, 300, 90, Mes, lstrlen(Mes));
		int pivot = 110;
		for (int i = 0; i < PLAYER_COUNT; ++i)
		{
			if (i == index - 1) continue;
			wsprintf(Mes, "상대 점수 : %d        ", score[i]);
			TextOut(hdc, 300, pivot, Mes, lstrlen(Mes));
			pivot += 20;
		}
	}
}

void GetTempFlip(int* tx, int* ty)
{
	int i, j;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			if (arCell[i][j].St == TEMPFLIP)
			{
				*tx = i;
				*ty = j;
				return;
			}
		}
	}
	*tx = -1;
}

int GetRemain()
{
	int i, j;
	int remain = 16;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			if (arCell[i][j].St == FLIP)
			{
				remain--;
			}
		}
	}
	return remain;
}

void DrawBitmap(HDC hdc,int x,int y,HBITMAP hBit)
{
	HDC MemDC;
	HBITMAP OldBitmap;
	int bx,by;
	BITMAP bit;

	MemDC=CreateCompatibleDC(hdc);
	OldBitmap=(HBITMAP)SelectObject(MemDC, hBit);

	GetObject(hBit,sizeof(BITMAP),&bit);
	bx=bit.bmWidth;
	by=bit.bmHeight;

	BitBlt(hdc,x,y,bx,by,MemDC,0,0,SRCCOPY);

	SelectObject(MemDC,OldBitmap);
	DeleteDC(MemDC);
}
