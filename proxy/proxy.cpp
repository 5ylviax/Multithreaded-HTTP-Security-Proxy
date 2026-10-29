#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

// CONSTANTS
#define MAX_SES 100
#define PROXY_PORT 9080
#define HTTP_PORT 4080
#define MAX_BUF 1200
#define SERVER_IP "127.0.0.1"

// GLOBAL VARIABLES
SOCKET server;
SOCKET client;
SOCKET client_s1[MAX_SES];
SOCKET client_s2[MAX_SES];

unsigned int session_id;

//Protoypes 

void Sending(void* dummy); // Traffic: Server <- Proxy <- Client
void Receving(void* dummy); // Traffic: Server -> Proxy -> Client
int Find_Available_Session(void); // returns the index of the first available session, or -1 if none are available


int main(void)
{
	WSADATA wsaData;
	SOCKET server_s1;
	SOCKET server_s2;


	int status;
	int addr_len1;
	int addr_len2;

	struct sockaddr_in server_addr1;
	struct sockaddr_in server_addr2;

}

/*
* WSAStartup
socket
bind to port 9080
listen
accept browser connection
create another socket
connect it to 127.0.0.1:4080
start two forwarding threads
continue accepting more browser connections
*/