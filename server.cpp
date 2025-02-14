#include <bits/stdc++.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <algorithm>

using namespace std;

#define GREEN "\033[32m"
#define RED "\033[31m"
#define RESET "\033[0m"

#define MAX_CLIENTS 5
#define BUFFER_SIZE 256
#define BUFFER_BYTES 256

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
            printf("server-client connection established. !!\n");
    }

    void closeServer(int clientSocket)
    {
        close(clientSocket);
    }
} serverObject;

char *msgParser(msgType command, string message, int sockSender)
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

    return &msg[0];
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

void privateMessage(vector<int> &sockReceiver, char *msg)
{
    ssize_t Nsend;
    for (auto it : sockReceiver)
    {
        Nsend = write(it, msg, BUFFER_BYTES);
        if (Nsend < 0)
        {
            /*error */
        }
    }
}

void broadcast(int sockSender, char *message)
{
    ssize_t Nsend;
    for (auto it : chatRoom)
    {
        if (it.second != sockSender)
        {
            Nsend = write(it.second, message, BUFFER_BYTES);
            if (Nsend < 0)
            {
                /*error */
            }
        }
    }
}

void globalChat(char *message)
{
    ssize_t Nsend;
    for (auto it : chatRoom)
    {
        Nsend = write(it.second, message, BUFFER_BYTES);
        if (Nsend < 0)
        {
            /*error */
        }
    }
}

// to be added : Users Not Present char* message maker
char *notPresentMsg(vector<string> &privateAliasNotFound)
{
    string msg = "Users: ";
    for (auto username : privateAliasNotFound)
    {
        msg += username;
        msg += ", ";
    }

    msg += "were not found in the Chat Room.";
    return &msg[0];
}

void userNotPresent(vector<string> &privateAliasNotFound, int sockSender)
{
    ssize_t Nsend;
    char *message = notPresentMsg(privateAliasNotFound); // Users Not Present char* message maker called here
    Nsend = write(sockSender, message, BUFFER_BYTES);
    if (Nsend < 0)
    {
        /*error */
    }
}

void chatting(int sockSender, char *buffer)
{
    ssize_t n_send, n_rec;
    string message;
    msgType command;
    vector<int> privateSocketNo;
    vector<string> privateAliasNotFound;

    bzero(buffer, BUFFER_BYTES);
    buffer = msgParser(CONNECT, "", sockSender);
    globalChat(buffer);
    pthread_mutex_unlock(&chatRoomMutex);

    bool disconnectFlag = false;
    while (!disconnectFlag)
    {
        privateSocketNo.clear();
        privateAliasNotFound.clear();
        bzero(buffer, BUFFER_BYTES);
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
}

void clientAlias(int socketNumber, char *buffer)
{
    ssize_t receivedByteSize, sentByteSize;
    string name;
    bzero(buffer, BUFFER_BYTES);
takename:
    sentByteSize = write(socketNumber, "Enter Alias:", 16);
    receivedByteSize = read(socketNumber, buffer, BUFFER_BYTES);
    if (receivedByteSize <= 0)
        goto takename;
    for (auto it : clientList)
    {
        if (it.second == name)
        {
            sentByteSize = write(socketNumber, "Alias already taken.", 16);
            goto takename;
        }
    }
    clientList[socketNumber] = name;
}

void *handleClient(void *socketDescription)
{
    int socketNumber = *(int *)socketDescription;
    char buffer[BUFFER_SIZE];
    string message;
    ssize_t receivedByteSize, sentByteSize;
    clientAlias(socketNumber, buffer);
    while (true)
    {
        bzero(buffer, BUFFER_BYTES);
        receivedByteSize = read(socketNumber, buffer, BUFFER_BYTES);
        if (receivedByteSize < 0)
        {
            break;
        }
        message = buffer;
        if (message == "CONNECT")
        {
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
        pthread_create(&clientThread, NULL, handleClient, (void *)serverObject.connfd);
        pthread_detach(clientThread); // to free resources on client termination
    }
    serverObject.closeServer(serverObject.sockfd);
    return 0;
}