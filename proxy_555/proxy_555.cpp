/*
* Silvia Saavedra 
* CS 447
* PROJECT 2
* DATE: 07/26/2026
* Student last three: 555
*/
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <stdlib.h>

#pragma comment(lib, "ws2_32.lib")

// CONSTANTS
#define MAX_SES 100
#define PROXY_PORT 9080 // listen to browser connections on this port
#define HTTP_PORT 4080 // connet to the real web server on port this port
#define MAX_BUF 1200
#define SERVER_IP "127.0.0.1" // set to the real web server's IP address
#define PROHIBITED_IP "0.0.0.0" // set to the client IP address to block

// Each session has two connected sockets , one for the browser and one for the HTTP server
SOCKET client_s1[MAX_SES]; // browser <-> proxy socket
SOCKET client_s2[MAX_SES]; // proxy <-> web-server socket

int connection_count = 0; // number of active connections

//prototype 
void Sending(void* dummy); // Traffic: Server <- Proxy <- Client
void Receving(void* dummy); // Traffic: Server -> Proxy -> Client
void Forbidden(void* dummy); 
int Find_Available_Session(void); // returns the index of the first available session, or -1 if none are available

int main(void)
{
	WSADATA wsaData; 

	SOCKET server_s1; // C1: listenign socket for browser connections
	SOCKET server_s2; // 
	SOCKET browser_s; // C3 : one per accepted browser connection

	SOCKADDR_IN addr1; // proxy listening address 
	SOCKADDR_IN addr2; // real web server address
	SOCKADDR_IN browser_addr; // filled in by accept()

	int browser_addr_len; // length of the browser address structure
	char lpszMessage[100]; // informatonal message buffer
	int i; // loop counter
	int session_id; // index of the session for the current connection

	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
	{
		printf("WSAStartup() failed: %d\n", WSAGetLastError());
		return 1;
	}

	for (i = 0; i < MAX_SES; i++)
	{
		client_s1[i] = INVALID_SOCKET;
		client_s2[i] = INVALID_SOCKET;
	}

	server_s1 = socket(AF_INET, SOCK_STREAM, 0);

	if (server_s1 != INVALID_SOCKET)
	{
		addr1.sin_family = AF_INET;
		addr1.sin_port = htons(PROXY_PORT);
		addr1.sin_addr.s_addr = htonl(INADDR_ANY); 

		if (bind(server_s1, (LPSOCKADDR)&addr1, sizeof(addr1)) != SOCKET_ERROR)
		{
			// listen for connections (queueing up to 3 connections)
			if (listen(server_s1, SOMAXCONN) != SOCKET_ERROR)
			{
				printf("Proxy: listening on port %d\n", PROXY_PORT);

				while (1)
				{
					browser_addr_len = sizeof(browser_addr);

					browser_s = accept(server_s1, (LPSOCKADDR)&browser_addr, &browser_addr_len);

					if (browser_s == INVALID_SOCKET)
					{
						sprintf(lpszMessage, "accept() generated error %d", WSAGetLastError());
						printf("%s\n", lpszMessage);
						continue;
					}

					sprintf(lpszMessage, "Proxy: connection accepted from %s:%d", inet_ntoa(browser_addr.sin_addr), ntohs(browser_addr.sin_port));
					printf("%s\n", lpszMessage);

					// Milestone #5 
					if (strcmp(inet_ntoa(browser_addr.sin_addr), PROHIBITED_IP) == 0)
					{
						printf("Prohibited IP detected: %s\n", inet_ntoa(browser_addr.sin_addr));
						_beginthread(Forbidden, 0, (void*)(UINT_PTR)browser_s);
						continue;
					}
					// Milestone #2-4
					// creating new socket to connect to the real web 
					session_id = Find_Available_Session();
					if (session_id == -1)
					{
						printf("No available session, closing connection\n");
						closesocket(browser_s);
						continue;

					}
					connection_count++;

					printf("Connecting client's IP: %s\n", inet_ntoa(browser_addr.sin_addr));
					printf("Session ID: %d\n", connection_count);

					server_s2 = socket(AF_INET, SOCK_STREAM, 0);

					if (server_s2 == INVALID_SOCKET)
					{
						printf("socket() generated error %d\n", WSAGetLastError());
						closesocket(browser_s);
						continue;
					}
					
					addr2.sin_family = AF_INET;
					addr2.sin_port = htons(HTTP_PORT);
					addr2.sin_addr.s_addr = inet_addr(SERVER_IP); // connect to the real web server on localhost

					if (connect(server_s2, (LPSOCKADDR)&addr2, sizeof(addr2)) == SOCKET_ERROR)
					{
						printf("connect() generated error %d\n", WSAGetLastError());
						closesocket(browser_s);
						closesocket(server_s2);
						continue;
					}

					client_s1[session_id] = browser_s;
					client_s2[session_id] = server_s2;

					// spawn two threads to handle the traffic in both directions
					_beginthread(Sending, 0, (void*)(UINT_PTR) session_id); // thread for traffic: client -> proxy -> server
					_beginthread(Receving, 0, (void*)(UINT_PTR)session_id); // thread for traffic: server -> proxy -> client

				}
			}
			else
			{
				sprintf(lpszMessage, "listen() generated error %d", WSAGetLastError());
				printf("%s\n", lpszMessage);
			}
		}
		else
		{
			sprintf(lpszMessage, "bind() generated error %d", WSAGetLastError());
			printf("%s\n", lpszMessage);
		}
	}
	else
	{
		sprintf(lpszMessage, "socket() generated error %d", WSAGetLastError());
		printf("%s\n", lpszMessage);
	}
	closesocket(server_s1);
	WSACleanup();
	return 0; 
}
// client(C3) --> proxy --> server
	//receive          // send
void Sending(void* dummy)
{
	int session_id = (int)(UINT_PTR)dummy;
	SOCKET from = client_s1[session_id]; // browser socket
	SOCKET to = client_s2[session_id]; // web server socket

	char in_buf[MAX_BUF];
	int nBytesRecv;
	int nSent;
	int nBytesSent;

	printf("Sending thread started for session %d\n", session_id);

	while (1)
	{
		nBytesRecv = recv(from, in_buf, MAX_BUF, 0);
		if (nBytesRecv == SOCKET_ERROR) 
		{
			//printf("recv() failed %d\n", WSAGetLastError());
			break;
		}
		if (nBytesRecv == 0)
		{
			printf("Connection closed by client\n");
			break;
		}

		nSent = 0;
		while (nSent < nBytesRecv)
		{
			nBytesSent = send(to, in_buf + nSent, nBytesRecv - nSent, 0);

			if (nBytesSent == SOCKET_ERROR || nBytesSent== 0)
			{
				break;
			}
			nSent += nBytesSent;
		}

		if (nSent < nBytesRecv)
		{
			break;
		}
	}
	//printf("Sending thread for session %d exiting\n", session_id);
	_endthread();
}
// Server (C2) --> proxy --> client 
	//receive            // send 
void Receving(void* dummy)
{
	int session_id = (int)(UINT_PTR)dummy;
	SOCKET from = client_s2[session_id]; // web server socket
	SOCKET to = client_s1[session_id]; // browser socket

	char int_buf[MAX_BUF];
	int nBytesRecv;
	int nSent;
	int nBytesSent;

	printf("Receving thread started\n");
	while (1)
	{
		nBytesRecv = recv(from, int_buf, MAX_BUF, 0);

		if (nBytesRecv == SOCKET_ERROR)
		{
			//printf("recv() failed %d\n", WSAGetLastError());
			break;
		}
		if (nBytesRecv == 0)
		{
			printf("Connection closed by server\n");
			break;
		}
		nSent = 0;
		while (nSent < nBytesRecv)
		{
			nBytesSent = send(to, int_buf + nSent, nBytesRecv - nSent, 0);
			if (nBytesSent == SOCKET_ERROR || nBytesSent == 0)
			{
				break;
			}
			nSent += nBytesSent;
		}
		if (nSent < nBytesRecv)
		{
			break;
		}
	}
	shutdown(client_s1[session_id], SD_BOTH);
	closesocket(client_s2[session_id]); // Close C2
	closesocket(client_s1[session_id]);// Close C3

	client_s1[session_id] = INVALID_SOCKET;
	client_s2[session_id] = INVALID_SOCKET;

	printf("Receiving thread ended for session %d\n", session_id);
	_endthread();
}

void Forbidden(void* dummy)
{
	SOCKET browser_s = (SOCKET)(UINT_PTR)dummy;

	char  in_buf[MAX_BUF];
	char  header[128];
	FILE* fp; // file pointer for the 403 file
	int   bodyLen;       
	char* body; // pointer to the body of the 403 file
	int   nSent;
	int   nBytesSent;
	int   headerLen;

	printf("Forbidden thread started (socket %d)\n", (int)browser_s);

	// recv the HTTP GET first, then discard it
	recv(browser_s, in_buf, MAX_BUF, 0);

	// locally open the 403 file
	fp = fopen("HTTP_403.html", "rb");

	if (fp == NULL)
	{
		printf("Cannot open HTTP_403.html\n");
		closesocket(browser_s);
		_endthread();
	}

	fseek(fp, 0, SEEK_END);
	bodyLen = (int)ftell(fp);
	rewind(fp);

	body = (char*)malloc(bodyLen);

	if (bodyLen <= 0 || body == NULL || fread(body, 1, bodyLen, fp) != (size_t)bodyLen)
	{
		printf("Cannot read HTTP_403.html\n");
		fclose(fp);
		free(body);
		closesocket(browser_s);
		_endthread();
	}

	fclose(fp);

	// Reply message + blank line
	strcpy(header, "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n\r\n");
	headerLen = (int)strlen(header);

	nSent = 0;
	while (nSent < headerLen)
	{
		nBytesSent = send(browser_s, header + nSent, headerLen - nSent, 0);
		if (nBytesSent == SOCKET_ERROR || nBytesSent == 0) break;
		nSent += nBytesSent;
	}

	// Contents of the file
	nSent = 0;
	while (nSent < bodyLen)
	{
		nBytesSent = send(browser_s, body + nSent, bodyLen - nSent, 0);
		if (nBytesSent == SOCKET_ERROR || nBytesSent == 0) break;
		nSent += nBytesSent;
	}

	free(body);
	closesocket(browser_s);     // socket disconnection signal

	printf("Forbidden thread ended (socket %d)\n", (int)browser_s);
	_endthread();
}

int Find_Available_Session(void)
{
	for (int i = 0; i < MAX_SES; i++)
	{
		if (client_s1[i] == INVALID_SOCKET && client_s2[i] == INVALID_SOCKET)
		{
			return i;
		}
	}
	return -1;
}