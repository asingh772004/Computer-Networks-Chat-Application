// Standard C++ Libraries
#include <iostream>  // For standard I/O operations
#include <vector>    // For std::vector (if needed)
#include <map>       // For std::map
#include <algorithm> // For std::find, std::remove, etc. (if needed)
#include <cstring>   // For memset(), strcpy(), etc.

// POSIX & System Libraries
#include <unistd.h> // For close(), read(), write(), etc.
#include <csignal>  // For handling signals (optional, if used)

// Networking Libraries
#include <sys/socket.h> // For socket functions (socket(), bind(), listen(), accept(), etc.)
#include <netinet/in.h> // For sockaddr_in structure
#include <netdb.h>      // For getaddrinfo(), gethostbyname(), etc.

// No threading library is needed since select() is used.

using namespace std;

#define RESET "\033[0m"
#define RED "\033[31m"    // Red color
#define GREEN "\033[32m"  // Green color
#define YELLOW "\033[33m" // Yellow color
#define CYAN "\033[36m"   // Cyan color

#define MAX_CLIENTS 5
#define BUFFER_SIZE 4096

enum msgType
{
    BROADCAST,
    CONNECT,
    DISCONNECT,
    PRIVATE,
    EXIT
};

// Global maps to manage client aliases.
// clientList: maps socket number to alias (empty if not set)
// chatRoom: maps alias to socket number (only when client has joined the chat)
map<int, string> clientList;
map<string, int> chatRoom;

// Function declarations
string msgParser(msgType command, string message, int sockSender);
void privateMsgParser(string &message, vector<int> &privateSocketNo, vector<string> &privateAliasNotFound);
msgType commandHandler(string &message, int sockSender, vector<int> &privateSocketNo, vector<string> &privateAliasNotFound);
void privateMessage(const vector<int> &sockReceiver, string message);
void broadcast(int sockSender, string message);
void globalChat(string message);
string notPresentMsg(const vector<string> &privateAliasNotFound);
void userNotPresent(const vector<string> &privateAliasNotFound, int sockSender);
bool isAliasTaken(const string &alias);
void handleClientAlias(int clientSock, char *buffer);

string msgParser(msgType command, string message, int sockSender)
{
    string msg = "";
    string username = clientList[sockSender];
    switch (command)
    {
    case CONNECT:
        msg += username;
        msg += " has joined the ChatRoom.\n";
        break;
    case DISCONNECT:
    case EXIT:
        msg += username;
        msg += " has left the ChatRoom.\n";
        break;
    case PRIVATE:
        msg += "[" + username + " (private)]: ";
        msg += message;
        msg += "\n";
        break;
    case BROADCAST:
        msg += "[" + username + "]: ";
        msg += message;
        msg += "\n";
        break;
    }
    return msg;
}

void privateMsgParser(string &message, vector<int> &privateSocketNo, vector<string> &privateAliasNotFound)
{
    int index = 0;
    while (index < message.size() && message[index] == '@')
    {
        string username;
        index++; // skip the '@'
        while (index < message.size() && message[index] != ' ')
        {
            username.push_back(message[index]);
            index++;
        }
        if (chatRoom.find(username) != chatRoom.end())
        {
            privateSocketNo.push_back(chatRoom[username]);
        }
        else
        {
            privateAliasNotFound.push_back(username);
        }
        while (index < message.size() && message[index] == ' ')
            index++;
    }
    if (index < message.size())
        message = message.substr(index);
    else
        message = "";
}

msgType commandHandler(string &message, int sockSender, vector<int> &privateSocketNo, vector<string> &privateAliasNotFound)
{
    msgType command = BROADCAST;
    if (!message.empty() && message[0] == '@')
    {
        command = PRIVATE;
        privateMsgParser(message, privateSocketNo, privateAliasNotFound);
        return command;
    }
    else if (message.size() >= 7 && message.substr(0, 7) == "CONNECT")
    {
        command = CONNECT;
        return command;
    }
    else if (message.size() >= 10 && message.substr(0, 10) == "DISCONNECT")
    {
        command = DISCONNECT;
        return command;
    }
    else if (message.size() >= 4 && message.substr(0, 4) == "EXIT")
    {
        command = EXIT;
        return command;
    }
    return command;
}

void privateMessage(const vector<int> &sockReceiver, string message)
{
    for (auto clientSocketNo : sockReceiver)
    {
        send(clientSocketNo, message.c_str(), message.size(), 0);
    }
}

void broadcast(int sockSender, string message)
{
    for (auto clientDetails : chatRoom)
    {
        if (clientDetails.second != sockSender)
        {
            send(clientDetails.second, message.c_str(), message.size(), 0);
        }
    }
}

void globalChat(string message)
{
    for (auto clientDetails : chatRoom)
    {
        send(clientDetails.second, message.c_str(), message.size(), 0);
    }
}

string notPresentMsg(const vector<string> &privateAliasNotFound)
{
    string msg = "";
    for (size_t i = 0; i < privateAliasNotFound.size(); i++)
    {
        if (i != 0)
            msg += ", ";
        msg += privateAliasNotFound[i];
    }
    msg += " were not found in the Chat Room.\n";
    return msg;
}

void userNotPresent(const vector<string> &privateAliasNotFound, int sockSender)
{
    if (privateAliasNotFound.empty())
        return;
    string message = notPresentMsg(privateAliasNotFound);
    send(sockSender, message.c_str(), message.size(), 0);
}

bool isAliasTaken(const string &alias)
{
    for (auto it : clientList)
    {
        if (it.second == alias)
            return true;
    }
    return false;
}

void handleClientAlias(int clientSock, char *buffer)
{
    string name;
    bool reEnterAlias = true;
    while (reEnterAlias)
    {
        reEnterAlias = false;
        string prompt = "Enter Alias: ";
        send(clientSock, prompt.c_str(), prompt.size(), 0);
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recv(clientSock, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead <= 0)
            continue;
        buffer[bytesRead] = '\0';
        name = string(buffer);
        name.erase(remove(name.begin(), name.end(), '\n'), name.end());
        name.erase(remove(name.begin(), name.end(), '\r'), name.end());
        if (isAliasTaken(name))
        {
            string taken = "Alias already taken.\n";
            send(clientSock, taken.c_str(), taken.size(), 0);
            reEnterAlias = true;
        }
    }
    clientList[clientSock] = name;
    cout << YELLOW << "Assigned Socket " << clientSock << " : " << name << RESET << endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << RED << "Port Number is missing" << RESET << endl;
        exit(0);
    }
    int port = atoi(argv[1]);

    // Create the listening socket.
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0)
    {
        cout << RED << "Socket Creation Failed" << RESET << endl;
        exit(0);
    }
    else
    {
        cout << GREEN << "Socket was successfully created." << RESET << endl;
    }

    // Set socket options.
    int opt = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cout << RED << "setsockopt failed" << RESET << endl;
        exit(0);
    }

    // Bind the socket.
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(serverSock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << RED << "Server socket bind failed." << RESET << endl;
        exit(0);
    }
    else
    {
        cout << GREEN << "Server binded successfully." << RESET << endl;
    }

    // Listen on the socket.
    if (listen(serverSock, MAX_CLIENTS) < 0)
    {
        cout << RED << "Server listen failed" << RESET << endl;
        exit(0);
    }
    else
    {
        cout << GREEN << "Server is listening" << RESET << endl;
    }

    cout << string(50, '-') << endl;

    // Set up select() variables.
    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_ZERO(&read_fds);

    FD_SET(serverSock, &master_set);
    int fdmax = serverSock;

    char buffer[BUFFER_SIZE];

    // Main loop using select().
    while (true)
    {
        read_fds = master_set;
        int activity = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0)
        {
            cout << RED << "Select error" << RESET << endl;
            break;
        }
        for (int i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == serverSock)
                {
                    // Accept new connection.
                    sockaddr_in cli_addr;
                    socklen_t clilen = sizeof(cli_addr);
                    int newSock = accept(serverSock, (struct sockaddr *)&cli_addr, &clilen);
                    if (newSock < 0)
                    {
                        cout << RED << "Server accept failed" << RESET << endl;
                        continue;
                    }
                    else
                    {
                        cout << GREEN << "Server-Client Connection Established on socket " << newSock << RESET << endl;
                    }
                    if ((int)clientList.size() >= MAX_CLIENTS)
                    {
                        cout << RED << "Maximum Number of Clients Reached" << RESET << endl;
                        string fullMsg = "Server is full. Try again later.\n";
                        send(newSock, fullMsg.c_str(), fullMsg.size(), 0);
                        close(newSock);
                        continue;
                    }
                    FD_SET(newSock, &master_set);
                    if (newSock > fdmax)
                        fdmax = newSock;
                    clientList[newSock] = "";
                    // Prompt for alias.
                    handleClientAlias(newSock, buffer);
                }
                else
                {
                    // Data from an existing client.
                    memset(buffer, 0, BUFFER_SIZE);
                    int bytesRead = recv(i, buffer, BUFFER_SIZE - 1, 0);
                    if (bytesRead <= 0)
                    {
                        if (bytesRead == 0)
                            cout << YELLOW << "Socket " << i << " hung up." << RESET << endl;
                        else
                            cout << RED << "recv error on socket " << i << RESET << endl;

                        // Broadcast disconnection if client was in chat.
                        if (chatRoom.find(clientList[i]) != chatRoom.end())
                        {
                            string leaveMsg = msgParser(DISCONNECT, "", i);
                            globalChat(leaveMsg);
                            chatRoom.erase(clientList[i]);
                        }
                        close(i);
                        FD_CLR(i, &master_set);
                        clientList.erase(i);
                    }
                    else
                    {
                        buffer[bytesRead] = '\0';
                        string message(buffer);
                        message.erase(remove(message.begin(), message.end(), '\n'), message.end());
                        message.erase(remove(message.begin(), message.end(), '\r'), message.end());

                        if (clientList[i] == "")
                        {
                            handleClientAlias(i, buffer);
                        }
                        else if (chatRoom.find(clientList[i]) == chatRoom.end())
                        {
                            // Client must type "CONNECT" to join the chat room.
                            if (message.size() >= 7 && message.substr(0, 7) == "CONNECT")
                            {
                                chatRoom[clientList[i]] = i;
                                string joinMsg = msgParser(CONNECT, "", i);
                                globalChat(joinMsg);
                                cout << joinMsg;
                                string confirm = "You have joined the chat room.\n";
                                send(i, confirm.c_str(), confirm.size(), 0);
                            }
                            else
                            {
                                string prompt = "Type CONNECT to join the chat room.\n";
                                send(i, prompt.c_str(), prompt.size(), 0);
                            }
                        }
                        else
                        {
                            // Process chat messages.
                            vector<int> privateSocketNo;
                            vector<string> privateAliasNotFound;
                            msgType command = commandHandler(message, i, privateSocketNo, privateAliasNotFound);
                            string parsedMsg = msgParser(command, message, i);
                            cout << CYAN << "\tSending: " << parsedMsg << RESET << endl;
                            switch (command)
                            {
                            case BROADCAST:
                                broadcast(i, parsedMsg);
                                break;
                            case PRIVATE:
                                privateMessage(privateSocketNo, parsedMsg);
                                userNotPresent(privateAliasNotFound, i);
                                break;
                            case EXIT:
                            case DISCONNECT:
                            {
                                string alias = clientList[i];
                                globalChat(parsedMsg);
                                close(i);
                                FD_CLR(i, &master_set);
                                clientList.erase(i);
                                chatRoom.erase(alias);
                                break;
                            }
                            case CONNECT:
                            {
                                string info = "You are already in the chat room.\n";
                                send(i, info.c_str(), info.size(), 0);
                                break;
                            }
                            }
                        }
                    }
                }
            }
        }
    }
    close(serverSock);
    return 0;
}