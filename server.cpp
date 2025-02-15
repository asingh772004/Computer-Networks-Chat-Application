// Standard C++ Libraries
#include <iostream>  // For standard I/O operations
#include <vector>    // For std::vector (if needed)
#include <map>       // For std::map
#include <algorithm> // For std::find, std::remove, etc. (if needed)
#include <cstring>   // For memset(), strcpy(), etc.

// POSIX & System Libraries
#include <unistd.h> // For close(), read(), write(), etc.
#include <fcntl.h>  // For fcntl() (optional, for non-blocking sockets)
#include <csignal>  // For handling signals (optional, if used)

// Networking Libraries
#include <sys/socket.h> // For socket functions (socket(), bind(), listen(), accept(), etc.)
#include <netinet/in.h> // For sockaddr_in structure
#include <netdb.h>      // For getaddrinfo(), gethostbyname(), etc.

// Threading Library
#include <pthread.h> // For pthreads (multithreading)

using namespace std;

#define GREEN "\033[32m"
#define RED "\033[31m"
#define RESET "\033[0m"

#define MAX_CLIENTS 5
#define BUFFER_SIZE 256

typedef enum msgType
{
    BROADCAST,
    CONNECT,
    DISCONNECT,
    PRIVATE
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
            cout << "Client Socket Creation Failed.Exiting..." << endl;
            exit(0);
        }
        else
            cout << "Client Socket was successfully created." << endl;
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
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
            cout << "Server socket bind failed." << endl;
            exit(0);
        }
        else
            cout << "Server binded successfully." << endl;
    }

    void serverListen()
    {
        listenid = listen(sockfd, 20);
        if (listenid != 0)
        {
            cout << "server listen failed." << endl;
            exit(0);
        }
        else
            cout << "server is listening." << endl;
    }

    void acceptClient()
    {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (connfd < 0)
        {
            printf("server accept failed...\n");
            exit(0);
        }
        else
        {
            printf("server-client connection established. !!\n");
            int flags = fcntl(connfd, F_GETFL, 0);
            fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
        }
    }

    void closeServer(int clientSocket)
    {
        close(clientSocket);
    }

    pair<ssize_t, string> recieveMessage(int clientSockNo, char *buffer, ssize_t bytesRead)
    {
        bzero(buffer, 256);
        bytesRead = read(clientSockNo, buffer, sizeof(buffer));
        string message(buffer);
        return {bytesRead, message};
    }

    ssize_t sendMessage(int clientSockNo, string message)
    {
        ssize_t bytesSent;
        char *msgPtr = &message[0];
        bytesSent = write(clientSockNo, msgPtr, message.size());
        return bytesSent;
    }
} serverObject;

string msgParser(msgType command, string message, int sockSender)
{
    string msg;
    string username = clientList[sockSender];
    switch (command)
    {
    case CONNECT:
        msg += username;
        msg += " has joined the ChatRoom";
        break;

    case DISCONNECT:
        msg += username;
        msg += " has left the ChatRoom";
        break;

    case PRIVATE:
        msg += "[" + username + "] ";
        msg += message;
        break;

    case BROADCAST:
        msg += "[" + username + "is Broadcasting] ";
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
        index++;
        while (index < message.size() && message[index] != ' ')
        {
            username += message[index];
            index++;
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
        index++;
    }

    message = message.substr(index);
    return;
}

msgType commandHandler(string &message, int sockSender, vector<int> &privateSocketNo, vector<string> &privateAliasNotFound)
{
    msgType command = BROADCAST;

    if (message[0] == '@')
    {
        command = PRIVATE;
        privateMsgParser(message, privateSocketNo, privateAliasNotFound);
        return command;
    }
    else if (message.size() >= 7 && message.substr(0, 7) == "CONNECT")
    {
        command = CONNECT;
        message = message.substr(7);
        return command;
    }
    else if (message.size() >= 10 && message.substr(0, 10) == "DISCONNECT")
    {
        command = DISCONNECT;
        message = message.substr(10);
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
        if (Nsend < 0)
        {
            /*error */
        }
    }
    return;
}

void broadcast(int sockSender, char *message)
{
    ssize_t Nsend;
    for (auto clientDetails : chatRoom)
    {
        if (clientDetails.second != sockSender)
        {
            Nsend = serverObject.sendMessage(clientDetails.second, message);
            if (Nsend < 0)
            {
                /*error */
            }
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
        if (Nsend < 0)
        {
            /*error */
        }
    }
    return;
}

// to be added : Users Not Present char* message maker
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
    ssize_t Nsend;
    string message = notPresentMsg(privateAliasNotFound); // Users Not Present char* message maker called here
    Nsend = serverObject.sendMessage(sockSender, message);
    if (Nsend < 0)
    {
        /*error */
    }
    return;
}

void chatting(int sockSender, char *buffer)
{
    cout << "Chat Room accessed" << endl;
    ssize_t n_send, n_rec;
    string message;
    msgType command;
    vector<int> privateSocketNo;
    vector<string> privateAliasNotFound;

    bzero(buffer, BUFFER_SIZE);
    buffer = msgParser(CONNECT, "", sockSender);
    globalChat(buffer);
    pthread_mutex_unlock(&chatRoomMutex);

    bool disconnectFlag = false;
    while (!disconnectFlag)
    {
        privateSocketNo.clear();
        privateAliasNotFound.clear();
        bzero(buffer, BUFFER_SIZE);
        n_rec = read(sockSender, buffer, 256);
        message = buffer;
        command = commandHandler(message, sockSender, privateSocketNo, privateAliasNotFound);
        buffer = msgParser(command, message, sockSender);
        switch (command)
        {
        case BROADCAST:
            broadcast(sockSender, buffer);
            break;
        case PRIVATE:
            privateMessage(privateSocketNo, buffer);
            userNotPresent(privateAliasNotFound, sockSender);
            break;
        case DISCONNECT:
            globalChat(buffer);
            disconnectFlag = true;
            break;
        }
    }
    return;
}

void clientAlias(int socketNumber, char *buffer)
{
    ssize_t receivedByteSize, sentByteSize;
    string name;
    bool aliasAssigned = false;
    while (!aliasAssigned)
    {
        sentByteSize = write(socketNumber, "Enter Alias:", strlen("Enter Alias:"));
        bzero(buffer, BUFFER_SIZE);
        receivedByteSize = read(socketNumber, buffer, BUFFER_BYTES);
        name = buffer;
        if (receivedByteSize < 0)
        {
            continue;
        }
        for (auto it : clientList)
        {
            if (it.second == name)
            {
                sentByteSize = write(socketNumber, "Alias already taken.", strlen("Alias already taken."));
            }
        }
        aliasAssigned = true;
    }
    clientList[socketNumber] = name;
    return;
}

void *handleClient(void *socketDescription)
{
    int socketNumber = *(int *)socketDescription;
    char buffer[BUFFER_SIZE];
    string message;
    ssize_t receivedByteSize, sentByteSize;
    clientAlias(socketNumber, buffer);

    cout << "Running Server Loop" << endl;

    while (true)
    {

        cout << "In the loop" << endl;

        bzero(buffer, BUFFER_SIZE);
        receivedByteSize = read(socketNumber, buffer, BUFFER_BYTES);
        if (receivedByteSize < 0)
        {
            continue;
        }
        message = buffer;

        sentByteSize = write(socketNumber, "Recieved Message", strlen("Recieved Message"));

        cout << message << endl;
        cout << (message.substr(0, 7) == "CONNECT") << endl;
        if (message.substr(0, 7) == "CONNECT")
        {
            cout << "Connect Loop" << endl;
            pthread_mutex_lock(&chatRoomMutex);
            chatRoom[clientList[socketNumber]] = socketNumber;
            chatting(socketNumber, buffer);
        }
        else if (message == "EXIT")
        {
            break;
        }
    }
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
        cout << "Error: Port Number is missing\nExiting..." << endl;
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
            cout << "Maximum Number of Clients Reached" << endl;
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