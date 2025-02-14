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

#define MAX_CLIENTS 5
#define BUFFER_SIZE 256

int clientCount = 0;
pthread_mutex_t clientCountMutex = PTHREAD_MUTEX_INITIALIZER;
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
        bzero((sockaddr_in *)&serv_addr, sizeof(serv_addr));
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

void client_Alias(int socketNumber, char *buffer)
{
    ssize_t receivedByteSize, sentByteSize;
    string name;
    bzero(buffer, 256);
takename:
    sentByteSize = write(socketNumber, "Enter Alias:", 16);
    receivedByteSize = read(socketNumber, buffer, 255);
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
    char buffer[256];
    string message;
    ssize_t receivedByteSize, sentByteSize;
    client_Alias(socketNumber, buffer);
    while (1)
    {
        bzero(buffer, 256);
        receivedByteSize = read(socketNumber, buffer, 255);
        message = buffer;
        if (receivedByteSize <= 0)
        {
            break;
        }
        printf("Message from client: %s\n", buffer);
        sentByteSize = write(socketNumber, "Message received", 16);
        if (sentByteSize < 0)
        {
            break;
        }
    }
    cout << "Client Disconnected" << endl;
    close(socketNumber);
    free(socketDescription);
    pthread_mutex_lock(&clientCountMutex);
    clientCount--;
    pthread_mutex_unlock(&clientCountMutex);
    return 0;
}

void clientAlias(int socketNumber, char *buffer)
{
    ssize_t receivedByteSize, sentByteSize;
    string name;
    bzero(buffer, BUFFER_SIZE);
takename:
    sentByteSize = write(socketNumber, "Enter Alias:", 16);
    receivedByteSize = read(socketNumber, buffer, BUFFER_SIZE);
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