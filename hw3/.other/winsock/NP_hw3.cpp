#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"
#include <winsock2.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")

#define SERVER_PORT 7799
#define DATA_BUFSIZE 8192
#define WM_SOCKET_NOTIFY (WM_USER + 1)

typedef struct _SOCKET_INFORMATION {
	BOOL RecvPosted;
	CHAR Buffer[DATA_BUFSIZE];
	WSABUF DataBuf;
	SOCKET Socket;
	DWORD BytesSEND;
	DWORD BytesRECV;
	struct _SOCKET_INFORMATION *Next;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
int EditPrintf (HWND, TCHAR *, ...);
//=================================================================
//	Global Variables
//=================================================================
list<SOCKET> Socks;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	
	return DialogBox(hInstance, MAKEINTRESOURCE(ID_MAIN), NULL, MainDlgProc);
}

BOOL CALLBACK MainDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	WSADATA wsaData;

	static HWND hwndEdit;
	static SOCKET msock, ssock;
	static struct sockaddr_in sa;
	SOCKET Accept;
	LPSOCKET_INFORMATION SocketInfo;
	DWORD RecvBytes;
	DWORD SendBytes;
	DWORD Flags;

	int err;


	switch(Message) 
	{
		case WM_INITDIALOG:
			hwndEdit = GetDlgItem(hwnd, IDC_RESULT);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_LISTEN:

					WSAStartup(MAKEWORD(2, 0), &wsaData);

					//create master socket
					msock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

					if( msock == INVALID_SOCKET ) {
						EditPrintf(hwndEdit, TEXT("=== Error: create socket error ===\r\n"));
						WSACleanup();
						return TRUE;
					}

					err = WSAAsyncSelect(msock, hwnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE | FD_READ | FD_WRITE);

					if ( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(msock);
						WSACleanup();
						return TRUE;
					}

					//fill the address info about server
					sa.sin_family		= AF_INET;
					sa.sin_port			= htons(SERVER_PORT);
					sa.sin_addr.s_addr	= INADDR_ANY;

					//bind socket
					err = bind(msock, (LPSOCKADDR)&sa, sizeof(struct sockaddr));

					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: binding error ===\r\n"));
						WSACleanup();
						return FALSE;
					}

					err = listen(msock, 2);
		
					if( err == SOCKET_ERROR ) {
						EditPrintf(hwndEdit, TEXT("=== Error: listen error ===\r\n"));
						WSACleanup();
						return FALSE;
					}
					else {
						EditPrintf(hwndEdit, TEXT("=== Server START ===\r\n"));
					}

					break;
				case ID_EXIT:
					EndDialog(hwnd, 0);
					break;
			};
			break;

		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;

		case WM_SOCKET_NOTIFY:
			switch( WSAGETSELECTEVENT(lParam) )
			{
				case FD_ACCEPT:
					ssock = accept(msock, NULL, NULL);
					Socks.push_back(ssock);
					EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d), List size:%d ===\r\n"), ssock, Socks.size());
					break;
				case FD_READ:
					//Write your code for read event here.
					SocketInfo = GetSocketInformation(wParam);
					// Read data only if the receive buffer is empty
					if (SocketInfo->BytesRECV != 0)
					{
						SocketInfo->RecvPosted = TRUE;
						return 0;
					}
					else
					{
						SocketInfo->DataBuf.buf = SocketInfo->Buffer;
						SocketInfo->DataBuf.len = DATA_BUFSIZE;

						Flags = 0;
						if (WSARecv(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &RecvBytes,
							&Flags, NULL, NULL) == SOCKET_ERROR)
						{
							if (WSAGetLastError() != WSAEWOULDBLOCK)
							{
								printf("WSARecv() failed with error %d\n", WSAGetLastError());
								FreeSocketInformation(wParam);
								return 0;
							}
						}
						else // No error so update the byte count
						{
							printf("WSARecv() is OK!\n");
							SocketInfo->BytesRECV = RecvBytes;
						}
					}
					break;
				case FD_WRITE:
					//Write your code for write event here
					SocketInfo = GetSocketInformation(wParam);
					if (SocketInfo->BytesRECV > SocketInfo->BytesSEND)
					{
						SocketInfo->DataBuf.buf = SocketInfo->Buffer + SocketInfo->BytesSEND;
						SocketInfo->DataBuf.len = SocketInfo->BytesRECV - SocketInfo->BytesSEND;

						if (WSASend(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &SendBytes, 0,
							NULL, NULL) == SOCKET_ERROR)
						{
							if (WSAGetLastError() != WSAEWOULDBLOCK)
							{
								printf("WSASend() failed with error %d\n", WSAGetLastError());
								FreeSocketInformation(wParam);
								return 0;
							}
						}
						else // No error so update the byte count
						{
							printf("WSASend() is OK!\n");
							SocketInfo->BytesSEND += SendBytes;
						}
					}

					if (SocketInfo->BytesSEND == SocketInfo->BytesRECV)
					{
						SocketInfo->BytesSEND = 0;
						SocketInfo->BytesRECV = 0;
						// If a RECV occurred during our SENDs then we need to post an FD_READ notification on the socket
						if (SocketInfo->RecvPosted == TRUE)
						{
							SocketInfo->RecvPosted = FALSE;
							PostMessage(hwnd, WM_SOCKET, wParam, FD_READ);
						}
					}
					break;
				case FD_CLOSE:
					break;
			};
			break;
		
		default:
			return FALSE;


	};

	return TRUE;
}

int EditPrintf (HWND hwndEdit, TCHAR * szFormat, ...)
{
     TCHAR   szBuffer [1024] ;
     va_list pArgList ;

     va_start (pArgList, szFormat) ;
     wvsprintf (szBuffer, szFormat, pArgList) ;
     va_end (pArgList) ;

     SendMessage (hwndEdit, EM_SETSEL, (WPARAM) -1, (LPARAM) -1) ;
     SendMessage (hwndEdit, EM_REPLACESEL, FALSE, (LPARAM) szBuffer) ;
     SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0) ;
	 return SendMessage(hwndEdit, EM_GETLINECOUNT, 0, 0); 
}