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

// No threading library is needed since we're using select().

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

int clientCount = 0;
map<int, string> clientList; // Maps socket to alias (empty until assigned)
map<string, int> chatRoom;   // Maps alias to socket (only if in chat room)

class server
{
public:
    int port, sockfd, connectid, bindid, listenid;
    int connfd; // used temporarily during accept()
    struct sockaddr_in serv_addr, cli_addr;

    void getPort(char *argv[])
    {
        port = atoi(argv[1]);
    }

    void socketNumber()
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            cout << RED << "Socket Creation Failed" << RESET << endl;
            exit(0);
        }
        else
        {
            cout << GREEN << "Socket was successfully created." << RESET << endl;
        }
    }

    void socketBind()
    {
        bzero(&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htons(INADDR_ANY);
        serv_addr.sin_port = htons(port);
        bindid = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (bindid < 0)
        {
            cout << RED << "Server socket bind failed." << RESET << endl;
            exit(0);
        }
        else
            cout << GREEN << "Server binded successfully." << RESET << endl;
    }

    void serverListen()
    {
        listenid = listen(sockfd, 20);
        if (listenid != 0)
        {
            cout << RED << "Server listen failed" << RESET << endl;
            exit(0);
        }
        else
            cout << GREEN << "Server is listening" << RESET << endl;
    }

    // Accept a new client connection and return its socket descriptor.
    int acceptClient()
    {
        socklen_t clilen = sizeof(cli_addr);
        int newSock = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newSock < 0)
        {
            cout << RED << "Server accept failed" << RESET << endl;
            return -1;
        }
        else
        {
            cout << GREEN << "Server-Client Connection Established" << RESET << endl;
        }
        return newSock;
    }

    void closeServer(int clientSocket)
    {
        close(clientSocket);
    }

    // Receives a message from a client. It reads until a newline is found.
    pair<ssize_t, string> receiveMessage(int clientSockNo, char *buffer)
    {
        string message;
        ssize_t bytesRead;
        while (true)
        {
            bzero(buffer, BUFFER_SIZE);
            bytesRead = read(clientSockNo, buffer, BUFFER_SIZE - 1);
            if (bytesRead <= 0)
            {
                return {bytesRead, message};
            }
            buffer[bytesRead] = '\0';
            message.append(buffer);
            if (message.find('\n') != string::npos)
            {
                break;
            }
        }
        if (!message.empty())
            message.pop_back(); // remove trailing newline
        return {message.size(), message};
    }

    // Sends a message to the specified client.
    ssize_t sendMessage(int clientSockNo, string message)
    {
        ssize_t bytesSent;
        // Use message.size() rather than sizeof(message)
        bytesSent = write(clientSockNo, message.c_str(), message.size());
        return bytesSent;
    }
} serverObject;

string msgParser(msgType command, string message, int sockSender)
{
    string msg = "";
    string username = clientList[sockSender];
    switch (command)
    {
    case CONNECT:
        msg += username;
        msg += " has joined the ChatRoom\n";
        break;

    case DISCONNECT:
        msg += username;
        msg += " has left the ChatRoom\n";
        break;

    case EXIT:
        msg += username;
        msg += " has left the ChatRoom\n";
        break;

    case PRIVATE:
        msg += "[" + username + "] ";
        msg += message;
        msg += "\n";
        break;

    case BROADCAST:
        msg += "[" + username + ", to ALL] ";
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
        index++; // skip the @
        while (index < message.size() && message[index] != ' ')
        {
            username += message[index];
            index++; // build the username string
        }
        if (chatRoom.find(username) != chatRoom.end())
        {
            int sock = chatRoom[username];
            privateSocketNo.push_back(sock);
        }
        else
        {
            privateAliasNotFound.push_back(username);
        }
        index++; // skip the space
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

void privateMessage(vector<int> &sockReceiver, string message)
{
    ssize_t Nsend;
    for (auto clientSocketNo : sockReceiver)
    {
        Nsend = serverObject.sendMessage(clientSocketNo, message);
    }
}

void broadcast(int sockSender, string message)
{
    ssize_t Nsend;
    for (auto clientDetails : chatRoom)
    {
        if (clientDetails.second != sockSender)
        {
            Nsend = serverObject.sendMessage(clientDetails.second, message);
        }
    }
}

void globalChat(string message)
{
    ssize_t Nsend;
    for (auto clientDetails : chatRoom)
    {
        Nsend = serverObject.sendMessage(clientDetails.second, message);
    }
}

string notPresentMsg(vector<string> &privateAliasNotFound)
{
    string msg = "";
    for (auto username : privateAliasNotFound)
    {
        if (username != privateAliasNotFound[0])
            msg += ", ";
        msg += username;
    }
    msg += " were not found in the Chat Room.\n";
    return msg;
}

void userNotPresent(vector<string> &privateAliasNotFound, int sockSender)
{
    if (privateAliasNotFound.empty())
        return;
    string message = notPresentMsg(privateAliasNotFound);
    serverObject.sendMessage(sockSender, message);
}

// Processes alias assignment for a client that hasn't yet set an alias.
void clientAlias(int socketNumber, char *buffer)
{
    pair<ssize_t, string> receiveReturn;
    ssize_t receivedByteSize, sentByteSize;
    string name;
    bool reEnterAlias = true;
    while (reEnterAlias)
    {
        reEnterAlias = false;
        sentByteSize = serverObject.sendMessage(socketNumber, "Enter Alias: ");
        receiveReturn = serverObject.receiveMessage(socketNumber, buffer);
        receivedByteSize = receiveReturn.first;
        name = receiveReturn.second;
        if (receivedByteSize <= 0)
            return;
        // Remove any newline/carriage return characters.
        name.erase(remove(name.begin(), name.end(), '\n'), name.end());
        name.erase(remove(name.begin(), name.end(), '\r'), name.end());
        for (auto it : clientList)
        {
            if (it.second == name)
            {
                sentByteSize = serverObject.sendMessage(socketNumber, "Alias already taken.\n");
                reEnterAlias = true;
                break;
            }
        }
    }
    clientList[socketNumber] = name;
    sentByteSize = serverObject.sendMessage(socketNumber, "Alias Assigned\n");
    cout << YELLOW << "Assigned Socket " << socketNumber << " : " << name << RESET << endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << RED << "Port Number is missing" << RESET << endl;
        exit(0);
    }
    serverObject.getPort(argv);
    serverObject.socketNumber();
    if (serverObject.sockfd < 0)
    {
        exit(0);
    }
    serverObject.socketBind();
    if (serverObject.bindid < 0)
    {
        exit(0);
    }
    serverObject.serverListen();
    if (serverObject.listenid != 0)
    {
        exit(0);
    }
    cout << string(50, '-') << endl;

    // Set up select() variables.
    fd_set master_set, read_fds;
    FD_ZERO(&master_set);
    FD_ZERO(&read_fds);
    FD_SET(serverObject.sockfd, &master_set);
    int fdmax = serverObject.sockfd;

    char buffer[BUFFER_SIZE];

    // Main loop using select()
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
                // New connection on the server socket.
                if (i == serverObject.sockfd)
                {
                    int newSock = serverObject.acceptClient();
                    if (newSock < 0)
                        continue;
                    if (clientCount >= MAX_CLIENTS)
                    {
                        cout << RED << "Maximum Number of Clients Reached" << RESET << endl;
                        string fullMsg = "Server is full. Try again later.\n";
                        serverObject.sendMessage(newSock, fullMsg);
                        close(newSock);
                        continue;
                    }
                    FD_SET(newSock, &master_set);
                    if (newSock > fdmax)
                        fdmax = newSock;
                    clientCount++;
                    clientList[newSock] = ""; // Alias not assigned yet.
                    // Immediately prompt for alias.
                    serverObject.sendMessage(newSock, "Enter Alias: ");
                }
                else
                {
                    // Data from an existing client.
                    memset(buffer, 0, BUFFER_SIZE);
                    pair<ssize_t, string> receiveReturn = serverObject.receiveMessage(i, buffer);
                    ssize_t bytesRead = receiveReturn.first;
                    string message = receiveReturn.second;
                    if (bytesRead <= 0)
                    {
                        // Client disconnected.
                        cout << YELLOW << "Socket " << i << " hung up." << RESET << endl;
                        // If the client was in the chat room, broadcast the disconnection.
                        if (clientList.find(i) != clientList.end())
                        {
                            string alias = clientList[i];
                            if (chatRoom.find(alias) != chatRoom.end())
                            {
                                string leaveMsg = msgParser(DISCONNECT, "", i);
                                globalChat(leaveMsg);
                                chatRoom.erase(alias);
                            }
                        }
                        close(i);
                        FD_CLR(i, &master_set);
                        clientList.erase(i);
                        clientCount--;
                    }
                    else
                    {
                        // Remove newline/carriage return characters.
                        message.erase(remove(message.begin(), message.end(), '\n'), message.end());
                        message.erase(remove(message.begin(), message.end(), '\r'), message.end());

                        // If alias not yet assigned, treat the incoming message as the alias.
                        if (clientList[i] == "")
                        {
                            clientAlias(i, buffer);
                        }
                        else if (chatRoom.find(clientList[i]) == chatRoom.end())
                        {
                            // Client is not in the chat room.
                            if (message.size() >= 7 && message.substr(0, 7) == "CONNECT")
                            {
                                chatRoom[clientList[i]] = i;
                                string joinMsg = msgParser(CONNECT, "", i);
                                globalChat(joinMsg);
                                cout << joinMsg;
                                string confirm = "You have joined the chat room.\n";
                                serverObject.sendMessage(i, confirm);
                            }
                            else if (message.size() >= 4 && message.substr(0, 4) == "EXIT")
                            {
                                string exitMsg = msgParser(EXIT, "", i);
                                serverObject.sendMessage(i, exitMsg);
                                close(i);
                                FD_CLR(i, &master_set);
                                clientList.erase(i);
                                clientCount--;
                            }
                            else
                            {
                                // Not in chat room: simply acknowledge or prompt.
                                string prompt = "Type CONNECT to join the chat room or EXIT to disconnect.\n";
                                serverObject.sendMessage(i, prompt);
                            }
                        }
                        else
                        {
                            // Client is in the chat room: process chat commands.
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
                            case DISCONNECT:
                                globalChat(parsedMsg);
                                chatRoom.erase(clientList[i]);
                                break;
                            case EXIT:
                                globalChat(parsedMsg);
                                close(i);
                                FD_CLR(i, &master_set);
                                clientList.erase(i);
                                chatRoom.erase(clientList[i]); // (if present)
                                clientCount--;
                                break;
                            case CONNECT:
                                serverObject.sendMessage(i, "You are already in the chat room.\n");
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    serverObject.closeServer(serverObject.sockfd);
    return 0;
}