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

// Threading Library
#include <pthread.h> // For pthreads (multithreading)

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
pthread_mutex_t clientCountMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t chatRoomMutex = PTHREAD_MUTEX_INITIALIZER;
map<int, string> clientList;
map<string, int> chatRoom;

class server
{
public:
    int port, sockfd, connectid, bindid, listenid, connfd;
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

    void acceptClient()
    {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (connfd < 0)
        {
            cout << RED << "Server accept failed" << RESET << endl;
            exit(0);
        }
        else
        {
            cout << GREEN << "Server-Client Connection Established" << RESET << endl;
        }
    }

    void closeServer(int clientSocket)
    {
        close(clientSocket);
    }

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
        message.pop_back();
        return {message.size(), message};
    }

    ssize_t sendMessage(int clientSockNo, string message)
    {
        ssize_t bytesSent;
        char *msgPtr = &message[0];
        bytesSent = write(clientSockNo, msgPtr, sizeof(message));
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
        msg += " has joined the ChatRoom";
        break;

    case DISCONNECT:
    case EXIT:
        msg += username;
        msg += " has left the ChatRoom";
        break;

    case PRIVATE:
        msg += "[" + username + "] ";
        msg += message;
        break;

    case BROADCAST:
        msg += "[" + username + " is Broadcasting] ";
        msg += message;
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
        index++; // skip the @ and go to the next character
        while (index < message.size() && message[index] != ' ')
        {
            username += message[index];
            index++; // keep traversing to build username string
        }

        if (chatRoom.find(username) != chatRoom.end())
        {
            int portNo = chatRoom[username];
            privateSocketNo.push_back(portNo);
        }
        else
        {
            privateAliasNotFound.push_back(username);
        }
        index++; // skip the ' ' character and collect the message
    }

    if (message.size() >= index)
    {
        message = message.substr(index);
    }
    else
    {
        message = "";
    }
    return;
}

msgType commandHandler(string &message, int sockSender, vector<int> &privateSocketNo, vector<string> &privateAliasNotFound)
{
    msgType command = BROADCAST;

    if (message[0] == '@')
    {
        // cout<<CYAN << "\tPrivate Detected" <<RESET<< endl;
        command = PRIVATE;
        privateMsgParser(message, privateSocketNo, privateAliasNotFound);
        return command;
    }
    else if (message.size() >= 7 && message.substr(0, 7) == "CONNECT")
    {
        // cout <<CYAN<< "\tConnect Detected" << RESET<<endl;
        command = CONNECT;
        return command;
    }
    else if (message.size() >= 10 && message.substr(0, 10) == "DISCONNECT")
    {
        // cout <<CYAN<< "\tDisconnect Detected" <<RESET<< endl;
        command = DISCONNECT;
        return command;
    }
    else if (message.size() >= 4 && message.substr(0, 4) == "EXIT")
    {
        // cout << "\tEXIT DETECTED " << endl;
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
    return;
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
    return;
}

void globalChat(string message)
{
    ssize_t Nsend;
    for (auto clientDetails : chatRoom)
    {
        Nsend = serverObject.sendMessage(clientDetails.second, message);
    }
    return;
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

    msg += "were not found in the Chat Room.";
    return msg;
}

void userNotPresent(vector<string> &privateAliasNotFound, int sockSender)
{
    if (privateAliasNotFound.size() == 0)
    {
        return;
    }
    ssize_t Nsend;
    string message = notPresentMsg(privateAliasNotFound);
    Nsend = serverObject.sendMessage(sockSender, message);
    return;
}

bool chatting(int sockSender, char *buffer)
{
    pair<ssize_t, string> receiveReturn;
    ssize_t nSend, nRec;
    string message;
    msgType command;
    vector<int> privateSocketNo;
    vector<string> privateAliasNotFound;
    bool returnValue = false;

    message = msgParser(CONNECT, "", sockSender);
    globalChat(message);
    cout << message << endl;
    pthread_mutex_unlock(&chatRoomMutex);

    bool disconnectFlag = false;
    while (!disconnectFlag)
    {
        privateSocketNo.clear();
        privateAliasNotFound.clear();

        receiveReturn = serverObject.receiveMessage(sockSender, buffer);
        nRec = receiveReturn.first;
        message = receiveReturn.second;

        cout << clientList[sockSender] << ": " << message << endl;
        command = commandHandler(message, sockSender, privateSocketNo, privateAliasNotFound);
        message = msgParser(command, message, sockSender);
        cout << CYAN << "\tSending: " << message << RESET << endl;

        switch (command)
        {
        case BROADCAST:
            broadcast(sockSender, message);
            break;
        case PRIVATE:
            privateMessage(privateSocketNo, message);
            userNotPresent(privateAliasNotFound, sockSender);
            break;
        case EXIT:
            returnValue = true;
        case DISCONNECT:
            globalChat(message);
            disconnectFlag = true;
            break;
        }
    }
    return returnValue;
}

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
        if (receivedByteSize < 0)
        {
            continue;
        }
        for (auto it : clientList)
        {
            if (it.second == name)
            {
                sentByteSize = write(socketNumber, "Alias already taken.", strlen("Alias already taken."));
                reEnterAlias = true;
                continue;
            }
        }
    }
    clientList[socketNumber] = name;
    cout << YELLOW << "Assigned Socket " << socketNumber << " : " << name << RESET << endl;
    return;
}

void *handleClient(void *socketDescription)
{
    int socketNumber = *(int *)socketDescription;
    char buffer[BUFFER_SIZE];
    pair<ssize_t, string> receiveReturn;
    string message;
    ssize_t receivedByteSize, sentByteSize;
    bool isEXIT = false;

    clientAlias(socketNumber, buffer);

    while (!isEXIT || !(message.size() >= 4 && message.substr(0, 4) == "EXIT"))
    {
        receiveReturn = serverObject.receiveMessage(socketNumber, buffer);
        receivedByteSize = receiveReturn.first;
        message = receiveReturn.second;
        if (receivedByteSize < 0)
        {
            continue;
        }

        cout << YELLOW << clientList[socketNumber] << ": " << message << RESET << endl;
        sentByteSize = serverObject.sendMessage(socketNumber, "Recieved Message");

        if (message.substr(0, 7) == "CONNECT")
        {
            pthread_mutex_lock(&chatRoomMutex);
            chatRoom[clientList[socketNumber]] = socketNumber;
            isEXIT = chatting(socketNumber, buffer);
            chatRoom.erase(clientList[socketNumber]);
        }
        else if (isEXIT || (message.size() >= 4 && message.substr(0, 4) == "EXIT"))
        {
            clientList.erase(socketNumber);
            break;
        }
    }
    cout << YELLOW << clientList[socketNumber] << ": is EXITING" << RESET << endl;
    close(socketNumber);
    free(socketDescription);
    pthread_mutex_lock(&clientCountMutex);
    clientCount--;
    pthread_mutex_unlock(&clientCountMutex);
    pthread_exit(NULL);
    return NULL;
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
    pthread_t clientThread;
    while (true)
    {
        serverObject.acceptClient();
        if (serverObject.connfd < 0)
        {
            continue;
        }
        pthread_mutex_lock(&clientCountMutex);
        if (clientCount >= 5)
        {
            cout << RED << "Maximum Number of Clients Reached" << RESET << endl;
            pthread_mutex_unlock(&clientCountMutex);
            continue;
        }
        clientCount++;
        pthread_mutex_unlock(&clientCountMutex);
        int *newSock = new int(serverObject.connfd);
        pthread_create(&clientThread, NULL, handleClient, (void *)newSock);
        pthread_detach(clientThread); // to free resources on client termination
    }
    serverObject.closeServer(serverObject.sockfd);
    return 0;
}