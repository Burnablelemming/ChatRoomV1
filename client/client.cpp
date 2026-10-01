#include <stdio.h>
#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include "winsock2.h"
#include "client_commands.cpp"

#define SERVER_PORT  11972
#define MAX_LINE      256



using namespace std;

// ================ Helper Functions ================
void parseClientInput(char* buf, Commands* cmd, string* message) {
	stringstream ss(buf);
	string command;
	ss >> command;
	if (command == "login") {
		*cmd = CMD_LOGIN;
		getline(ss >> ws, *message);
	}
	else if (command == "logout") {
		*cmd = CMD_LOGOUT;
	}
	else if (command == "send") {
		*cmd = CMD_SEND_MESSAGE;
		getline(ss >> ws, *message);
	}
	else if (command == "newuser") {
		*cmd = CMD_CREATE_USER;
		getline(ss >> ws, *message);
	}
	else {
		*cmd = CMD_NONE;
	}
}

void main(int argc, char** argv) {

	if (argc < 2) {
		printf("\nUseage: client serverName\n");
		return;
	}

	// Initialize Winsock.
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return;
	}

	//translate the server name or IP address (128.90.54.1) to resolved IP address
	unsigned int ipaddr;
	// If the user input is an alpha name for the host, use gethostbyname()
	// If not, get host by addr (assume IPv4)
	if (isalpha(argv[1][0])) {   // host address is a name  
		hostent* remoteHost = gethostbyname(argv[1]);
		if (remoteHost == NULL) {
			printf("Host not found\n");
			WSACleanup();
			return;
		}
		ipaddr = *((unsigned long*)remoteHost->h_addr);
	}
	else //"128.90.54.1"
		ipaddr = inet_addr(argv[1]);


	// Create a socket.
	SOCKET s;
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return;
	}

	// Connect to a server.
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ipaddr;
	addr.sin_port = htons(SERVER_PORT);
	if (connect(s, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		printf("Failed to connect.\n");
		WSACleanup();
		return;
	}

	// Send and receive data.
	Commands cmd = CMD_NONE;
	printf("My chat room client. Version One.\n\n");
	while (cmd != CMD_LOGOUT) {
		// Prompt the user for input
		char buf[MAX_LINE];
		printf("Enter command: ");
		if (!fgets(buf, sizeof(buf), stdin)) {
			printf("Error reading input. Please try again.\n");
		}

		string message;
		parseClientInput(buf, &cmd, &message);

		switch (cmd) {
			case CMD_LOGIN:
				printf("Logging in with message: %s\n", message.c_str());
				// login(message);
				continue;
			case CMD_LOGOUT:
				printf("Logging out.\n");
				closesocket(s);
				WSACleanup();
				return;
			case CMD_SEND_MESSAGE:
				printf("Sending message: %s\n", message.c_str());
				//sendMessage(message);
				break;
			case CMD_CREATE_USER:
				printf("Creating user with message: %s\n", message.c_str());
				//createUser(message)
				continue;
			default:
				printf("Unknown command. Please try again.\n");
				continue;
		}

		send(s, message.c_str(), strlen(message.c_str()), 0);
		int len = recv(s, buf, MAX_LINE, 0);
		buf[len] = 0;
		printf("Server says: %s\n", buf);
	}
	closesocket(s);
}