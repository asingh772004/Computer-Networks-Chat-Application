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
#define BUFFER_SIZE 100
int client_count = 0;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
map<int, string> Client_List;
map<string, int> Chat_Room;

typedef enum msgType
{
    BROADCAST,
    CONNECT,
    DISCONNECT,
    PRIVATE
};

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

    string readClient(int clientSocket)
    {
        // Read from the server
        char buffer[BUFFER_SIZE];
        bzero(buffer, sizeof(buffer));
        read(clientSocket, buffer, 8 * sizeof(buffer));
        string ans(buffer);
        return ans;
    }

    void writeClient(string message, int clientSocket)
    {
        // Write back to the server
        char *buffer = &message[0];
        write(clientSocket, buffer, 8 * sizeof(message));
    }

    void closeServer(int clientSocket)
    {
        close(clientSocket);
    }
};

char *msgParser(msgType command, string message, int sock_sender)
{
    string msg;
    string username = Client_List[sock_sender];
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

        if (Chat_Room.find(username) != Chat_Room.end())
        {
            int portNo = Chat_Room[username];
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

msgType commandHandler(string &message, int sock_sender, vector<int> &privateSocketNo, vector<string> &privateAliasNotFound)
{
    msgType command;

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

void Chatting(int sock_sender, char *buffer)
{
    ssize_t n_send, n_rec;
    vector<int> privateSocketNo;
    string message;
    msgType command;
    vector<string> privateAliasNotFound;
    bool disconnectFlag = false;
    while (!disconnectFlag)
    {
        n_rec = read(sock_sender, buffer, 256);
        message = buffer;
        command = commandHandler(message, sock_sender, privateSocketNo, privateAliasNotFound);
        buffer = msgParser(command, message, sock_sender);
        switch (command)
        {
        case BROADCAST:
            //
            break;
        case CONNECT:
            //
            break;
        case PRIVATE:
            //
            break;
        case DISCONNECT:
            //
            disconnectFlag = true;
            break;
        }
    }
}

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
    for (auto it : Client_List)
    {
        if (it.second == name)
        {
            sentByteSize = write(socketNumber, "Alias already taken.", 16);
            goto takename;
        }
    }
    Client_List[socketNumber] = name;
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
    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);
    return 0;
}