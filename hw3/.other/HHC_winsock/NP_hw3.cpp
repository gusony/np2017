#include <windows.h>
#include <list>
using namespace std;

#include "resource.h"
#include "np_project3.h"

#define SERVER_PORT 7799

#define WM_SOCKET_NOTIFY (WM_USER + 1)

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
				{
					ssock = accept(msock, NULL, NULL);
					Socks.push_back(ssock);
					EditPrintf(hwndEdit, TEXT("=== Accept one new client(%d), List size:%d ===\r\n"), ssock, Socks.size());
					err = WSAAsyncSelect(ssock, hwnd, WM_SOCKET_NOTIFY, FD_READ);
					if (err == SOCKET_ERROR) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(ssock);
						WSACleanup();
						return TRUE;
					}
					break;
				}
				case FD_READ:
				//Write your code for read event here.
					http_read_handler(ssock, hwndEdit);
					err = WSAAsyncSelect(ssock, hwnd, WM_SOCKET_NOTIFY, FD_WRITE);
					if (err == SOCKET_ERROR) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(ssock);
						WSACleanup();
						return TRUE;
					}
					break;
				case FD_WRITE:
				//Write your code for write event here
					http_write_handler(hwnd, ssock, hwndEdit);
					break;
				case FD_CLOSE:
					break;
			};
			break;

		case WM_SERVER_NOTIFY:
			switch (WSAGETSELECTEVENT(lParam))
			{
				int i, j, n;
				case F_CONNECTING:
				{
					int error = 0;
					if (getsockopt(wParam, SOL_SOCKET, SO_ERROR, (char *)&error, &n) < 0 || error != 0)
						/* non-blocking connect failed */
						return -1;

					err = WSAAsyncSelect(wParam, hwnd, WM_SERVER_NOTIFY, F_READING);
					if (err == SOCKET_ERROR) {
						EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
						closesocket(wParam);
						WSACleanup();
						return TRUE;
					}
					break;
				}
				case F_READING:
					for (i = 0; i < 5; i++)
						if (clients[i].sockfd == wParam)
							break;

					if (i == 5)
						return -1;

					n = recv(wParam, buff, MINBUFF - 1, 0);
					buff[n] = '\0';
					writeweb(ssock, i, buff, n, 0);

					if (strstr(buff, "% ") != NULL)
					{
						/* read finished */
						err = WSAAsyncSelect(wParam, hwnd, WM_SERVER_NOTIFY, F_WRITING);
						if (err == SOCKET_ERROR) {
							EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
							closesocket(wParam);
							WSACleanup();
							return TRUE;
						}
					}
					if (n == 0 && strncmp(clients[i].cmd, "exit", 4) == 0)
					{
						closesocket(clients[i].sockfd);
						fclose(clients[i].fp);
						free(clients[i].cmd);
						memset(clients + i, 0, sizeof(Client));
						clients[i].ip = NULL;
						clients[i].cmd = NULL;
						conn--;

						if (conn == 0)
						{
							WriteFooter(ssock);
							WSACleanup();
						}
					}
					break;
				case F_WRITING:
					char *pch, *sch;
					for (i = 0; i < 5; i++)
						if (clients[i].sockfd == wParam)
							break;

					if (i == 5)
						return -1;

					if (clients[i].NeedWrite <= 0)
					{
						if ((pch = strchr(clients[i].cmd, '\n')) == NULL)
						{
							readfile(i, hwndEdit);
							pch = strchr(clients[i].cmd, '\n');
						}

						if (pch == NULL)
							return -1;

						clients[i].NeedWrite = pch - clients[i].cmd + 1;
					}
					else /* Command not send finish */
						pch = strchr(clients[i].cmd, '\n');

					if (pch == NULL)
						return -1;

					sch = pch - clients[i].NeedWrite + 1;
					n = send(clients[i].sockfd, sch, clients[i].NeedWrite, 0);
					clients[i].NeedWrite -= n;

					if (clients[i].NeedWrite <= 0)
					{
						int cmd_length = pch - clients[i].cmd;
						for (j = 1; (pch - j) >= clients[i].cmd && *(pch - j) == '\r'; j++)
						{
							*(pch - j) = '\0';
							cmd_length--;
						}
						*(pch++) = '\0';
						writeweb(ssock, i, clients[i].cmd, cmd_length, 1);

						/* write finished */
						err = WSAAsyncSelect(wParam, hwnd, WM_SERVER_NOTIFY, F_READING);
						if (err == SOCKET_ERROR) {
							EditPrintf(hwndEdit, TEXT("=== Error: select error ===\r\n"));
							closesocket(wParam);
							WSACleanup();
							return TRUE;
						}

						if (strncmp(clients[i].cmd, "exit", 4) != 0)
						{
							for (j = 0; j < clients[i].buff_size; j++)
							{
								if ((pch - clients[i].cmd) < clients[i].buff_size)
									clients[i].cmd[j] = *(pch++);
								else
									clients[i].cmd[j] = '\0';
							}
							if (strncmp(clients[i].cmd, "exit", 4) == 0)
							{
								clients[i].cmd[4] = '\n';
								clients[i].cmd[5] = '\0';
							}
						}
					}
					break;
				case F_DONE:
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