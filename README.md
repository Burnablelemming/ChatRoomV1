# ChatRoomV1

Chat room project for CS 4850 Computer Networks I at the University of Missouri.

A simple TCP-based chat room client and server written in C++ using Winsock. This is Version 1, so only one client can connect to the server at a time. The server echoes messages back to the client rather than broadcasting them to other users.

Let's hope I remember how to code when making this project. Turns out I remembered enough.

### Setup

1. Clone the repository to your local machine.
2. Ensure you have Visual Studio installed with the **Desktop development with C++** workload.
3. Open the client and server solution files in Visual Studio.
4. Ensure the correct MSVC build tools and Windows SDK are installed.
5. Build both projects.
6. If a build fails, verify that the required MSVC toolset is installed or retarget the project to your installed toolset.

### Running the Application

1. Build and start the server application first.
2. Ensure the server is listening for incoming connections.
3. Build and start the client application in a separate terminal or Visual Studio instance.
4. The client connects to the server using TCP on `127.0.0.1`.
5. Enter commands into the client console to interact with the server.
6. Use `logout` to close the connection and exit the client. The server will continue running and wait for another client.
7. Pray that it works.

### Available Commands

| Command | Description |
|---|---|
| `login UserID Password` | Log in using an existing account. |
| `newuser UserID Password` | Create a new user account. |
| `send message` | Send a message to the server and receive an echo with your username. |
| `logout` | Log out and disconnect from the server. |

**Command restrictions:**

- Users must log in before sending messages or logging out.
- Users cannot log in again or create accounts while already logged in.
- Usernames must be between 3 and 32 characters.
- Passwords must be between 4 and 8 characters.
- Messages must be between 1 and 256 characters.
- Usernames and passwords are case-sensitive.

### Example Session

```text
newuser Mike Mike11
New user account created. Please login.

login Mike Mike11
login confirmed

send Hello everyone!
Mike: Hello everyone!

send This is my second message.
Mike: This is my second message.

logout
Mike left.
```

### Design Choices

I wanted to keep the client and server organized rather than putting all the networking, command processing, and validation directly into `main()`.

**1. Shared Protocol**

The client and server both include `protocol.h`, which defines the commands, status codes, and request/response structures.

I used a simple request/response protocol inspired by HTTP, with numeric status codes such as `200`, `400`, `401`, and `409`. This makes it easier to distinguish successful operations from different types of errors.

Requests and responses are serialized into strings and transmitted over TCP using a delimiter-based format.

For example:

```text
LOGIN|Tom Tom11
200|login confirmed
```

Each transmitted message ends with a newline so the receiver can determine when a complete message has arrived.

**2. Separate Client and Server Responsibilities**

The client handles user input, command validation, and displaying server responses.

The server handles authentication, user account management, message processing, and session authorization.

This keeps the responsibilities separated and prevents the client from directly accessing or modifying stored user accounts.

**3. Command Handlers**

Instead of implementing everything inside one large function, I separated the functionality into individual handlers for login, account creation, sending messages, and logout.

Commands are parsed and routed to their corresponding handlers, making the program easier to read, debug, and extend.

**4. Session Management**

Both the client and server maintain their own session state, including whether a user is logged in and their username.

The client uses this state to reject invalid commands before sending them, while the server independently enforces the same restrictions.

A successful logout ends the current connection, but the server continues running and accepts another client.

**5. TCP Communication**

The project uses Winsock with TCP sockets to establish a reliable connection between the client and server.

Because TCP is a byte stream rather than a message-based protocol, I implemented send and receive functions that handle partial transmissions and identify complete messages using newline delimiters.

This allows multiple commands to be processed over the same connection without reconnecting after every request at the cost of some performance.

**6. User Account Persistence**

User accounts are stored in a local `users.txt` file on the server.

The server checks for duplicate usernames before creating new accounts and appends new users to the file. If the file does not exist, it can be created when the first account is registered.

Accounts persist between server restarts.

### Limitations

This is the first version of the project, so there are a few intentional limitations:

- Only one client can connect and interact with the server at a time.
- Messages are echoed back rather than broadcast to other clients.
- User credentials are stored in plaintext in `users.txt`, as required by the assignment's simple file-based account system.
- The application is intended for local testing and educational use, not production deployment.

### Future Improvements

If this project were expanded, some possible improvements would include supporting multiple simultaneous clients using threads, broadcasting messages between connected users, improving credential storage, and adding more advanced chat room functionality.

### Technologies Used

- C++
- Winsock2
- TCP/IP
- Visual Studio
- Standard C++ libraries for file handling, string parsing, and input validation

---

Created for **CS 4850 – Computer Networks I**, Fall 2026.