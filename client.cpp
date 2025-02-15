// Standard C++ Libraries
#include <iostream>  // For standard I/O operations
#include <vector>    // For std::vector (if needed)
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

using namespace std;

#define RESET "\033[0m"
#define YELLOW "\033[33m" // yellow
#define RED "\033[31m"    // Red color
#define GREEN "\033[32m"  // Green color
#define WHITE "\033[37m"  // White color

#define BUFFER_SIZE 256
#define BUFFER_BYTES 8 * BUFFER_SIZE

pthread_mutex_t consoleLock = PTHREAD_MUTEX_INITIALIZER;

class client
{
public:
    int portno, sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];
    ssize_t bytesRead, bytesSent;

    void getPort(char *argv[])
    {
        portno = atoi(argv[2]);
    }

    void getSocketNo()
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            error("ERROR opening socket");
            exit(0);
        }
    }

    void getServer(char *argv[])
    {
        server = gethostbyname(argv[1]);
        if (server == NULL)
        {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }
    }

    void initServer()
    {
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);
    }

    void connectServer()
    {
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            error("ERROR connecting");
            exit(0);
        }
    }

    pair<ssize_t, string> recieveMessage()
    {
        bzero(buffer, 256);
        bytesRead = read(sockfd, buffer, BUFFER_BYTES);
        string message(buffer);
        return {bytesRead, message};
    }

    ssize_t sendMessage(string message)
    {
        char *msgPtr = &message[0];
        bytesSent = write(sockfd, msgPtr, 8 * sizeof(message));
        return bytesSent;
    }

    void closeClient()
    {
        close(sockfd);
    }

} clientObject;

void consoleStatement(string message)
{
    pthread_mutex_lock(&consoleLock);
    cout << message << endl;
    pthread_mutex_unlock(&consoleLock);
}

void *readHandler(void *args)
{
    pair<ssize_t, string> recieveReturn;
    string message;
    ssize_t bytesRead;
    while (true)
    {
        recieveReturn = clientObject.recieveMessage();
        bytesRead = recieveReturn.first;
        message = recieveReturn.second;
        if (bytesRead < 0)
        {
            consoleStatement("Error in Reading");
        }
        else if (bytesRead == 0)
        {
            consoleStatement("Server Disconnected");
            break;
        }
        else
        {
            consoleStatement(message);
        }
    }
    pthread_exit(NULL);
}

void *writeHandler(void *args)
{
    string message;
    while (true)
    {
        cin >> message;
        clientObject.sendMessage(message);
        if (message == "EXIT")
        {
            break;
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    clientObject.getPort(argv);
    clientObject.getSocketNo();
    clientObject.getServer(argv);
    clientObject.initServer();
    clientObject.connectServer();

    pthread_t readThread, writeThread;
    pthread_create(&readThread, NULL, readHandler, NULL);
    pthread_create(&writeThread, NULL, writeHandler, NULL);
    pthread_join(writeThread, NULL);
    pthread_cancel(readThread);

    clientObject.closeClient();
    return 0;
}