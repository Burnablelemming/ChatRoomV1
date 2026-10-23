#include <stdio.h>
#include <sstream>
#include <string>
#include <fstream>
#include "winsock2.h"
#include "../protocol.h"

#define SERVER_PORT   11972
#define MAX_PENDING   5
#define MAX_LINE      256
#define MAX_REQUEST   512

using namespace std;

const string USERS_FILE = "../users.txt";

// ================ Server Session ================

struct ServerSession {
    bool loggedIn = false;
    string username = "";
};

// ================ Helper Functions ================

// Parse a serialized client request into a ClientRequest struct.
ClientRequest parseRequest(const string& rawRequest) {
    ClientRequest request;

    size_t delimiter = rawRequest.find('|');

    if (delimiter == string::npos) {
        return request;
    }

    string command = rawRequest.substr(0, delimiter);
    request.body = rawRequest.substr(delimiter + 1);

    if (command == "LOGIN") {
        request.command = CMD_LOGIN;
    }
    else if (command == "LOGOUT") {
        request.command = CMD_LOGOUT;
    }
    else if (command == "SEND") {
        request.command = CMD_SEND_MESSAGE;
    }
    else if (command == "NEWUSER") {
        request.command = CMD_CREATE_USER;
    }

    return request;
}

// Check whether a command is permitted in the current server session.
bool isCommandAllowed(Commands command, const ServerSession& session) {
    if (session.loggedIn) {
        return command == CMD_SEND_MESSAGE || command == CMD_LOGOUT;
    }

    return command == CMD_LOGIN || command == CMD_CREATE_USER;
}

// Serialize a ServerResponse for transmission over TCP.
string serializeResponse(const ServerResponse& response) {
    return to_string(static_cast<int>(response.statusCode)) + "|" + response.message + "\n";
}

// Receive a complete newline-delimited request.
bool receiveRequest(SOCKET socket, string& rawRequest) {
    rawRequest.clear();

    char ch;

    while (true) {
        int bytesReceived = recv(socket, &ch, 1, 0);

        if (bytesReceived == SOCKET_ERROR) {
            printf("Error receiving request: %d\n", WSAGetLastError());
            return false;
        }

        if (bytesReceived == 0) {
            printf("Client disconnected.\n");
            return false;
        }

        if (ch == '\n') {
            return true;
        }

        rawRequest += ch;

        if (rawRequest.length() > MAX_REQUEST) {
            printf("Request exceeds maximum length.\n");
            return false;
        }
    }
}

// Send an entire serialized server response.
bool sendResponse(SOCKET socket, const ServerResponse& response) {
    string data = serializeResponse(response);

    int totalSent = 0;
    int dataLength = static_cast<int>(data.length());

    while (totalSent < dataLength) {
        int bytesSent = send(socket, data.c_str() + totalSent, dataLength - totalSent, 0);

        if (bytesSent == SOCKET_ERROR) {
            printf("Error sending response: %d\n", WSAGetLastError());
            return false;
        }

        if (bytesSent == 0) {
            printf("Connection closed while sending response.\n");
            return false;
        }

        totalSent += bytesSent;
    }

    return true;
}

// Validate login credentials against a simple users.txt file.
bool authenticateUser(const string& username, const string& password) {
    ifstream file(USERS_FILE);

    if (!file.is_open()) {
        printf("Error: Could not open users.txt\n");
        return false;
    }

    string line;

    while (getline(file, line)) {

        // Find the comma separating username and password.
        size_t comma = line.find(',');

        if (comma == string::npos) {
            continue;
        }

        // Extract username (skip opening parenthesis).
        string storedUsername = line.substr(1, comma - 1);

        // Extract password (skip comma and space, exclude closing parenthesis).
        string storedPassword = line.substr(comma + 2);

        if (!storedPassword.empty() && storedPassword.back() == ')') {
            storedPassword.pop_back();
        }

        // Compare credentials.
        if (storedUsername == username && storedPassword == password) {
            return true;
        }
    }

    return false;
}

// Check if a username already exists in users.txt.
bool userExists(const string& username) {
    ifstream file(USERS_FILE);

    if (!file.is_open()) {
        printf("Error: Could not open users.txt\n");
        return false;
    }

    string line;

    while (getline(file, line)) {
        size_t comma = line.find(',');

        if (comma == string::npos) {
            continue;
        }

        string storedUsername = line.substr(1, comma - 1);

        if (storedUsername == username) {
            return true;
        }
    }

    return false;
}

// ================ Command Handlers ================

// Handle login requests.
ServerResponse handleLogin(const ClientRequest& request, ServerSession& session) {
    stringstream ss(request.body);

    string username;
    string password;
    string extra;

    // Verify that exactly two arguments were supplied.
    if (!(ss >> username >> password) || (ss >> extra)) {
        return {STATUS_BAD_REQUEST, "Invalid login format."};
    }

	if (!authenticateUser(username, password)) {
		return { STATUS_UNAUTHORIZED, "Denied. User name or password incorrect." };
	}

    session.loggedIn = true;
    session.username = username;

    printf("%s login.\n", session.username.c_str());

    return {STATUS_OK, "login confirmed"};
}

// Placeholder for account creation.
ServerResponse handleCreateUser(const ClientRequest& request, ServerSession&) {
    stringstream ss(request.body);

    string username;
    string password;
    string extra;

    // Basic format check.
    if (!(ss >> username >> password) || (ss >> extra)) {
        return {STATUS_BAD_REQUEST, "Invalid newuser format."};
    }

    // Check for duplicate usernames.
    if (userExists(username)) {
        return {STATUS_CONFLICT, "Denied. User account already exists"};
    }

    // Append the new user.
    ofstream output(USERS_FILE, ios::app);

    if (!output.is_open()) {
        return {STATUS_INTERNAL_ERROR, "Unable to write to users file."};
    }

    output << "(" << username << ", " << password << ")\n";
    output.close();

    if (!output) {
        return {STATUS_INTERNAL_ERROR, "Failed to save new user."};
    }

    printf("New user account created\n");

    return {STATUS_CREATED, "New user account created. Please login."};
}

// Placeholder for sending chat messages.
ServerResponse handleSendMessage(const ClientRequest& request, ServerSession& session) {
    string message = session.username + ": " + request.body;

    printf("%s\n", message.c_str());

    return {STATUS_OK, message};
}

// Placeholder for logout.
ServerResponse handleLogout(const ClientRequest& request, ServerSession& session) {
    if (!request.body.empty()) {
        return {
            STATUS_BAD_REQUEST,
            "Logout does not accept arguments."
        };
    }
    string username = session.username;

    printf("%s logout\n", username.c_str());

    session.loggedIn = false;
    session.username = "";

    return {STATUS_OK, username + " left."};
}

// ================ Request Router ================

// Validate session permissions and route requests to command handlers.
ServerResponse handleRequest(const ClientRequest& request, ServerSession& session) {
    if (request.command == CMD_NONE) {
        return {STATUS_BAD_REQUEST, "Invalid command."};
    }

    if (!isCommandAllowed(request.command, session)) {
        return {STATUS_FORBIDDEN, "Command not allowed in current session."};
    }

    switch (request.command) {
    case CMD_LOGIN:
        return handleLogin(request, session);

    case CMD_CREATE_USER:
        return handleCreateUser(request, session);

    case CMD_SEND_MESSAGE:
        return handleSendMessage(request, session);

    case CMD_LOGOUT:
        return handleLogout(request, session);

    default:
        return {STATUS_BAD_REQUEST, "Invalid command."};
    }
}

// ================ Main ================

int main() {
    // Initialize Winsock.
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (iResult != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        return 1;
    }

    // Create a listening socket.
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Bind the socket.
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);

    if (bind(listenSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("bind() failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections.
    if (listen(listenSocket, MAX_PENDING) == SOCKET_ERROR) {
        printf("Error listening on socket.\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("My chat room server. Version One.\n\n");
   
    // Accept and process one connected client at a time.
    while (true) {

        SOCKET s = accept(listenSocket, NULL, NULL);

        if (s == INVALID_SOCKET) {
            printf("accept() error: %d\n", WSAGetLastError());
            break;
        }

        //printf("Client Connected.\n");

        // Each connection receives its own server-side session.
        ServerSession session;

        // Process multiple requests on the same TCP connection.
        while (true) {

            string rawRequest;

            // Receive the complete request.
            if (!receiveRequest(s, rawRequest)) {
                break;
            }

            // Deserialize the request.
            ClientRequest request = parseRequest(rawRequest);

            // Route the request and generate a response.
            ServerResponse response = handleRequest(request, session);

            // Send the complete response.
            if (!sendResponse(s, response)) {
                break;
            }

            // End the current client connection after successful logout.
            if (request.command == CMD_LOGOUT && response.statusCode == STATUS_OK) {
                break;
            }
        }

        closesocket(s);
    }

    closesocket(listenSocket);
    WSACleanup();

    return 0;
}
