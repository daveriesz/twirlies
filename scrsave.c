
#include <windows.h>
#include <scrnsave.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glu.h>

void RunSaver();

BOOL bSetupPixelFormat(HDC);

HWND TwHWnd = NULL;

LONG WINAPI ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDC   hdc;
	static HGLRC ghRC;
	static RECT  rc;
//	static UINT  uTimer;

	static DWORD dwTwThreadId;
	static HANDLE hTwThread;
	static int server_running = 0;

	TwHWnd = hWnd;

	switch(message)
	{
		case WM_MOUSEMOVE:
			return TRUE;
			break;
		case WM_CREATE:
			printf("WM_CREATE (hWnd = 0x%x)\n", hWnd);

//			uTimer = SetTimer(hWnd, 1, 1, NULL);

			hdc = GetDC(hWnd);
			if(!bSetupPixelFormat(hdc))
			{
				printf("display initialization error\n");
				PostQuitMessage(0);
			}

			ghRC = wglCreateContext(hdc);
			wglMakeCurrent(hdc, ghRC);
			tw_init();

//			RunSaver();

			break;
		case WM_ERASEBKGND:
			printf("WM_ERASEBKGND\n");
/*
			hdc = GetDC(hWnd);
			GetClientRect(hWnd, &rc);
			FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
			ReleaseDC(hWnd, hdc);
*/
			break;
		case WM_SIZE:
			printf("WM_SIZE\n");
			GetClientRect(hWnd, &rc);
			tw_reshape(rc.right, rc.bottom);
			break;
		case WM_TIMER:
			printf("WM_TIMER\n");
//			tw_display();
			break;
		case WM_PAINT:
			printf("WM_PAINT\n");
			if(!server_running)
			{
				server_running = 1; 
				RunSaver();
				exit(0);
			}
			break;
		case WM_DESTROY:
			printf("WM_DESTROY\n");
//			if(uTimer) KillTimer(hWnd, uTimer);
			break;
	}

	return DefScreenSaverProc(hWnd, message, wParam, lParam);
}

BOOL bSetupPixelFormat(HDC hdc)
{
	PIXELFORMATDESCRIPTOR pfd, *ppfd;
	int pixelformat;

	ppfd = &pfd;
	ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
	ppfd->nVersion = 1;
	ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;// | PFD_SINGLEBUFFER;
	ppfd->dwLayerMask = PFD_MAIN_PLANE;
	ppfd->iPixelType = PFD_TYPE_COLORINDEX;
	ppfd->cColorBits = 24;
	ppfd->cDepthBits= 32;
	ppfd->cAccumBits = 0;
	ppfd->cStencilBits = 0;

	pixelformat = ChoosePixelFormat(hdc, ppfd);

	if((pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0)
	{
		MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
		return FALSE;
	}

	if(SetPixelFormat(hdc, pixelformat, ppfd) == FALSE)
	{
		MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
		return FALSE;
	}

	return TRUE;
}

void RunSaver()
{
	MSG msg;

	PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
	while(WM_QUIT != msg.message)
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else tw_display();
	}
}


BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
	printf("RegisterDialogClasses\n");
	return TRUE;
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	printf("ScreenSaverConfigureDialog\n");

	if(message == WM_INITDIALOG)
	{
	}
	else if(message == WM_COMMAND)
	{
	}
	return TRUE;
}

