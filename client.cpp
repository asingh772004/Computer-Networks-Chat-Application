#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <csignal>
#include <netdb.h> // Include this for gethostbyname()

using namespace std;

#define RED "\033[31m" // red
#define RESET "\033[0m"
#define YELLOW "\033[33m" // yellow
#define RED "\033[31m"    // Red color
#define GREEN "\033[32m"  // Green color
#define WHITE "\033[37m"  // White color

int sockfd;
void receive_handler(char *buffer);

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void *receive_messages(void *arg)
{
    char buffer[256];
    while (true)
    {
        bzero(buffer, 256);
        ssize_t n = read(sockfd, buffer, 255);
        if (n < 0)
        {
            error("ERROR reading from socket");
        }
        receive_handler(buffer);
    }
    return nullptr;
}

void receive_handler(char *buffer)
{
    string message(buffer);

    if (message[0] == 'B')
    {
        cout << GREEN << " " << message.substr(1) << " " << RESET << endl;
    }
    else
    {
        cout << RED << " " << message.substr(1) << " " << RESET << endl;
    }
}

void *send_messages(void *arg)
{
    char buffer[256];
    while (true)
    {
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        cout << YELLOW << " : YOU \n"
             << RESET;
        ssize_t n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
        {
            error("ERROR writing to socket");
        }
    }
    return nullptr;
}

int main(int argc, char *argv[])
{
    int portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(1);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting");
    }

    pthread_t send_thread, receive_thread;
    pthread_create(&send_thread, nullptr, send_messages, nullptr);
    pthread_create(&receive_thread, nullptr, receive_messages, nullptr);

    pthread_join(send_thread, nullptr);
    pthread_join(receive_thread, nullptr);

    close(sockfd);
    return 0;
}