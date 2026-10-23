/*
* Name: Carl Carver
* Student ID: 14451972
* Course: CS 4850 - Computer Networks I
* Date: October 23, 2026
* Project: ChatRoom Version 1
* File: client.cpp
*
*/

#include <stdio.h>
#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include "winsock2.h"
#include "../protocol.h"

// Define constants for server connection and buffer sizes
#define SERVER_PORT  11972
#define MAX_LINE      512
#define MAX_RESPONSE  512

// Define constants for username and password length constraints
#define MAX_USERNAME_LENGTH 32
#define MIN_USERNAME_LENGTH 3
#define MAX_PASSWORD_LENGTH 8
#define MIN_PASSWORD_LENGTH 4


using namespace std;

// This struct represents the state of a client session, including whether the user is logged in and their username.
struct ClientSession {
	bool loggedIn = false;
	string username = "";
};

// ================ Helper Functions ================
// This function parses a raw input string from the client into a ClientRequest struct, extracting the command and body.
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

	getline(ss >> ws, request.body);

	return request;
}

// This function checks if a given command is allowed based on the current session state (logged in or not).
bool isCommandAllowed(Commands cmd, const ClientSession& session) {

	if (cmd == CMD_NONE) {
		return false;
	}

	if (session.loggedIn) {
		return cmd == CMD_SEND_MESSAGE || cmd == CMD_LOGOUT;
	}

	return cmd == CMD_LOGIN || cmd == CMD_CREATE_USER;
}

// This function handles the login command by validating the username and password format and length.
bool validateCredentials(const ClientRequest& request) {
	stringstream ss(request.body);

	string username;
	string password;
	string extra;

	// Extract username and password
	if (!(ss >> username >> password)) {
		printf("Invalid login format. Usage: login UserID Password\n");
		return false;
	}

	// Check for additional arguments
	if (ss >> extra) {
		printf("Invalid login format. Too many arguments.\n");
		return false;
	}

	// Validate username length
	if (username.length() < MIN_USERNAME_LENGTH || username.length() > MAX_USERNAME_LENGTH) {
		printf("Invalid username. Must be 3-32 characters.\n");
		return false;
	}

	// Validate password length
	if (password.length() < MIN_PASSWORD_LENGTH || password.length() > MAX_PASSWORD_LENGTH) {
		printf("Invalid password. Must be 4-8 characters.\n");
		return false;
	}

	return true;
}

// This function serializes a ClientRequest into a string format suitable for sending to the server.
string serializeRequest(const ClientRequest& request) {
	string command;

	switch (request.command) {
	case CMD_LOGIN:
		command = "LOGIN";
		break;

	case CMD_LOGOUT:
		command = "LOGOUT";
		break;

	case CMD_SEND_MESSAGE:
		command = "SEND";
		break;

	case CMD_CREATE_USER:
		command = "NEWUSER";
		break;

	default:
		return "";
	}

	return command + "|" + request.body + "\n";
}

// This function sends a serialized ClientRequest to the server over a given socket, ensuring that all bytes are sent.
bool sendRequest(SOCKET socket, const ClientRequest& request) {
	string data = serializeRequest(request);

	// Check for invalid serialization
	if (data.empty()) {
		printf("Error: Invalid request.\n");
		return false;
	}

	int totalSent = 0;
	int dataLength = static_cast<int>(data.length());

	// Continue until all bytes have been sent
	while (totalSent < dataLength) {
		int bytesSent = send(socket, data.c_str() + totalSent, dataLength - totalSent, 0);

		if (bytesSent == SOCKET_ERROR || bytesSent == 0) {
			printf("Error sending request: %d\n", WSAGetLastError());
			return false;
		}

		totalSent += bytesSent;
	}

	return true;
}

// This function receives a response from the server over a given socket, reading until a newline character is encountered.
bool receiveResponse(SOCKET socket, string& response) {
	response.clear();

	char ch;

	while (true) {
		int bytesReceived = recv(socket, &ch, 1, 0);

		if (bytesReceived == SOCKET_ERROR) {
			printf("Error receiving response: %d\n", WSAGetLastError());
			return false;
		}

		if (bytesReceived == 0) {
			printf("Server disconnected.\n");
			return false;
		}

		// End of response
		if (ch == '\n') {
			return true;
		}

		response += ch;

		if (response.length() > MAX_RESPONSE) {
			printf("Error: Server response too long.\n");
			return false;
		}
	}
}

// This function parses a raw server response string into a ServerResponse struct, extracting the status code and message.
ServerResponse parseServerResponse(const string& rawResponse) {
	ServerResponse response;

	size_t delimiter = rawResponse.find('|');

	if (delimiter == string::npos) {
		response.message = "Invalid server response.";
		return response;
	}

	string status = rawResponse.substr(0, delimiter);
	response.message = rawResponse.substr(delimiter + 1);

	try {
		size_t parsedLength = 0;
		int code = stoi(status, &parsedLength);

		// Make sure the status code is valid and the entire string was parsed
		if (parsedLength != status.length() || status.length() != 3 || code < 100 || code > 599) {
			response.message = "Invalid server response.";
			return response;
		}

		response.statusCode = static_cast<StatusCode>(code);
	}
	catch (const std::exception&) {
		response.message = "Invalid server response.";
	}

	return response;
}

// ================ Command Handlers ================

// This function handles the login command by validating credentials, sending the request to the server, and processing the response.
bool handleLogin(SOCKET socket, const ClientRequest& request, ClientSession& session) {

	if (!validateCredentials(request)) {
		return true;
	}

	// Return false to terminate connection only if server operations fail.
	if (!sendRequest(socket, request)) {
		return false;
	}

	string rawResponse;

	// Same here, return false to terminate connection only if server operations fail.
	if (!receiveResponse(socket, rawResponse)) {
		return false;
	}

	ServerResponse response = parseServerResponse(rawResponse);

	printf("%s\n", response.message.c_str());

	if (response.statusCode == STATUS_OK) {
		stringstream ss(request.body);
		ss >> session.username;

		session.loggedIn = true;
	}
	return true;
}

// This function handles the newuser command by validating the username and password format, sending the request to the server, and processing the response.
bool handleCreateUser(SOCKET socket, const ClientRequest& request) {

	// Validate the username and password format.
	if (!validateCredentials(request)) {
		return true;
	}

	// Send the request to the server.
	if (!sendRequest(socket, request)) {
		return false;
	}

	// Receive the server's response.
	string rawResponse;

	if (!receiveResponse(socket, rawResponse)) {
		return false;
	}

	// Parse and display the response.
	ServerResponse response = parseServerResponse(rawResponse);

	printf("%s\n", response.message.c_str());

	return true;
}

// This function handles the send command by validating the message length, sending the request to the server, and processing the response.
bool handleSendMessage(SOCKET socket, const ClientRequest& request) {

	// Validate message length.
	if (request.body.empty() || request.body.length() > 256) {
		printf("Message must be between 1 and 256 characters.\n");
		return true;
	}

	// Send request to server.
	if (!sendRequest(socket, request)) {
		return false;
	}

	// Receive response.
	string rawResponse;

	if (!receiveResponse(socket, rawResponse)) {
		return false;
	}

	// Parse response.
	ServerResponse response = parseServerResponse(rawResponse);

	printf("%s\n", response.message.c_str());

	return true;
}

// This function handles the logout command by sending the request to the server, processing the response, and resetting the client session state if successful.
bool handleLogout(SOCKET socket, const ClientRequest& request, ClientSession& session) {

	// Send logout request.
	if (!sendRequest(socket, request)) {
		return false;
	}

	// Receive server response.
	string rawResponse;

	if (!receiveResponse(socket, rawResponse)) {
		return false;
	}

	// Parse response.
	ServerResponse response = parseServerResponse(rawResponse);

	printf("%s\n", response.message.c_str());

	// Reset the client session after successful logout.
	if (response.statusCode == STATUS_OK) {
		session.loggedIn = false;
		session.username = "";
	}

	return true;
}

// ================ Main ================

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

	bool connectionActive = true;

	while (connectionActive) {
		// Prompt the user for input
		char buf[MAX_LINE];
		
		if (!fgets(buf, sizeof(buf), stdin)) {
			printf("Error reading input.\n");
			break;
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

		// Route the command to the appropriate handler function
		switch (request.command) {
			case CMD_LOGIN:
				if (!handleLogin(s, request, session)) {
					connectionActive = false;
				}
				break;

			case CMD_LOGOUT:
				if (!handleLogout(s, request, session)) {
					connectionActive = false;
				}
				else if (!session.loggedIn) {
					connectionActive = false;
				}
				break;

			case CMD_SEND_MESSAGE:
				if (!handleSendMessage(s, request)) {
					connectionActive = false;
				}
				break;

			case CMD_CREATE_USER:
				if (!handleCreateUser(s, request)) {
					connectionActive = false;
				}
				break;

			default:
				break;
		}
	}
	closesocket(s);
	WSACleanup();
}