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

// This struct represents the state of a client session, including whether the user is logged in and their username.
struct ClientSession {
	bool loggedIn = false;
	string username = "";
};

// This struct represents the message sent from the client to the server, including the command and any associated payload.
struct ClientRequest {
	Commands command = CMD_NONE;
	string payload = "";
};

// ================ Helper Functions ================
ClientRequest parseClientInput(const string& input) {
	stringstream ss(input);
	string command;

	ClientRequest request;

	ss >> command;

	if (command == "login") {
		request.command = CMD_LOGIN;
	}
	else if (command == "logout") {
		request.command = CMD_LOGOUT;
	}
	else if (command == "send") {
		request.command = CMD_SEND_MESSAGE;
	}
	else if (command == "newuser") {
		request.command = CMD_CREATE_USER;
	}
	else {
		request.command = CMD_NONE;
	}

	getline(ss >> ws, request.payload);

	return request;
}

bool isCommandAllowed(Commands cmd, const ClientSession& session) {

	if (cmd == CMD_NONE) {
		return false;
	}

	if (session.loggedIn) {
		return cmd == CMD_SEND_MESSAGE || cmd == CMD_LOGOUT;
	}

	return cmd == CMD_LOGIN || cmd == CMD_CREATE_USER;
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
	ClientSession session;
	
	printf("My chat room client. Version One.\n\n");
	while (true) {
		// Prompt the user for input
		char buf[MAX_LINE];
		printf("Enter command: ");
		if (!fgets(buf, sizeof(buf), stdin)) {
			printf("Error reading input. Please try again.\n");
		}

		string message;

		// Parse the user input into a clientRequest struct
		ClientRequest request = parseClientInput(buf);

		
		if (request.command == CMD_NONE) {
			printf("Unknown command. Please try again.\n");
			continue;
		}

		// Check if the command is allowed based on the current session state
		if (!isCommandAllowed(request.command, session)) {
			if (!session.loggedIn) {
				printf("Denied. Please login first.\n");
			}
			else {
				printf("Denied. Already logged in.\n");
			}
			continue;
		}

		switch (request.command) {
		case CMD_LOGIN:
			printf("Logging in with: %s\n", request.payload.c_str());
			break;

		case CMD_LOGOUT:
			printf("Logging out.\n");
			break;

		case CMD_SEND_MESSAGE:
			printf("Sending message: %s\n", request.payload.c_str());
			break;

		case CMD_CREATE_USER:
			printf("Creating user: %s\n", request.payload.c_str());
			break;

		default:
			break;
		}

		send(s, message.c_str(), strlen(message.c_str()), 0);
		int len = recv(s, buf, MAX_LINE, 0);
		buf[len] = 0;
		printf("Server says: %s\n", buf);
	}
	closesocket(s);
}