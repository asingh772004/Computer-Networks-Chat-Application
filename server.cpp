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

#define Server_ID "10.10.209.140"
#define Port_No 4269

#define MAX_CLIENTS 5
int client_count = 0;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;
map<int, string> Client_List;
map<string, int> Chat_Room;

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void trim(string &s)
{
    int n = s.length();
    int i = 0;
    for (; i < n; i++)
    {
        if (s[i] != ' ')
            break;
    }
    s = s.substr(i);
}

bool isExit(string s)
{
    int n = s.length();
    int space = s.find_first_of(' ');
    s = s.substr(0, space);
    return (s == "/exit");
}

void client_Alias(int sock, char *buffer)
{

    ssize_t Nrec, Nsend;
    string name;
    bzero(buffer, 256);
takename:
    Nsend = write(sock, "Enter Alias:", 16);
    Nrec = read(sock, buffer, 255);
    if (Nrec <= 0)
        goto takename;
    for (auto it : Client_List)
    {
        if (it.second == name)
        {
            Nsend = write(sock, "Alias already taken.", 16);
            goto takename;
        }
    }
    Client_List[sock] = name;
}

void join_Chat(int sock, char *buffer)
{
    ssize_t Nsend;
    bzero(buffer, 256);
    buffer = (char *)Client_List[sock][0] + " has joined the chat";
    Nsend = write(sock, buffer, 16);
}

void *handleClient(void *socket_desc)
{
    int sock = *(int *)socket_desc;
    char buffer[256];
    string message;
    ssize_t Nrec, Nsend;
    client_Alias(sock, buffer);
    while (1)
    {
        bzero(buffer, 256);
        Nrec = read(sock, buffer, 255);
        message = buffer;
        trim(message);
        if (Nrec <= 0 || isExit(message))
        {
            break;
        }
        printf("Message from client: %s\n", buffer);
        Nsend = write(sock, "Message received", 16);
        if (Nsend < 0)
        {
            break;
        }
    }
    cout << "Client Disconnected" << endl;
    close(sock);
    free(socket_desc);
    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);
    return 0;
}

int createSocket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cout << "Socket Creation Failed..exiting" << endl;
        exit(0);
    }
    else
        cout << "Socket was successfully created" << endl;
    return sockfd;
}

int main()
{
    int sockfd, newsockfd;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    ssize_t n;

    sockfd = createSocket();
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(Port_No);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        {
            error("ERROR on accept");
            continue;
        }

        pthread_mutex_lock(&client_count_mutex);
        if (client_count >= MAX_CLIENTS)
        {
            pthread_mutex_unlock(&client_count_mutex);
            cout << "Maximum clients reached. Connection refused." << endl;
            close(newsockfd);
            continue;
        }
        client_count++;
        pthread_mutex_unlock(&client_count_mutex);

        pthread_t thread_id;
        int *new_sock = (int *)malloc(sizeof(int));
        *new_sock = newsockfd;
        if (pthread_create(&thread_id, NULL, handleClient, (void *)new_sock) < 0)
        {
            error("Could not create thread");
            return 1;
        }
        pthread_detach(thread_id);
    }
    close(sockfd);
    return 0;
}