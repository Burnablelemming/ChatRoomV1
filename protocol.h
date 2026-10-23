// This header file defines the protocol for communication between the chat client and server, including command types, status codes, and request/response structures.

#pragma once

#include <string>

// Commands supported by the chat room protocol
enum Commands {
    CMD_NONE,
    CMD_LOGIN,
    CMD_LOGOUT,
    CMD_SEND_MESSAGE,
    CMD_CREATE_USER
};

// HTTP-inspired response status codes
enum StatusCode : int {
    STATUS_OK = 200,
    STATUS_CREATED = 201,
    STATUS_BAD_REQUEST = 400,
    STATUS_UNAUTHORIZED = 401,
    STATUS_FORBIDDEN = 403,
    STATUS_NOT_FOUND = 404,
    STATUS_CONFLICT = 409,
    STATUS_INTERNAL_ERROR = 500
};

// Request sent from client to server
struct ClientRequest {
    Commands command = CMD_NONE;
    std::string body = "";
};

// Response sent from server to client
struct ServerResponse {
    StatusCode statusCode = STATUS_INTERNAL_ERROR;
    std::string message = "";
};