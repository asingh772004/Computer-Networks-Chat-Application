// Standard C++ Libraries
#include <iostream>  // For standard I/O operations
#include <vector>    // For std::vector (if needed)
#include <algorithm> // For std::find, std::remove, etc. (if needed)
#include <string>    // For std::string
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

// custom libraries for Chat Terminal
#include "terminal.h"

using namespace std;

#define RESET "\033[0m"
#define RED "\033[31m"    // Red color
#define GREEN "\033[32m"  // Green color
#define YELLOW "\033[33m" // Yellow color

#define BUFFER_SIZE 4096

terminal terminalObject;

void leaveGracefully()
{
    terminalObject.closeTerminal();
    exit(0);
}

class client
{
public:
    int portno, sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
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
            cout << RED << "Could not open Socket" << RESET << endl;
            leaveGracefully();
        }
    }

    void getServer(char *argv[])
    {
        server = gethostbyname(argv[1]);
        if (server == NULL)
        {
            cout << RED << "Could not get Server" << RESET << endl;
            leaveGracefully();
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
            cout << RED << "Could not connect to Server" << RESET << endl;
            leaveGracefully();
        }
    }

    void closeClient()
    {
        close(sockfd);
    }

    pair<ssize_t, string> recvAll()
    {
        string message = "";
        const size_t CHUNK_SIZE = 32;
        char chunk[CHUNK_SIZE];
        ssize_t totalBytesRead = 0;

        while (true)
        {
            bzero(chunk, CHUNK_SIZE);
            ssize_t bytesRead = read(sockfd, chunk, CHUNK_SIZE - 1);

            if (bytesRead < 0)
            {
                if (errno == EINTR)
                    continue;    // Interrupted by signal, retry
                return {-1, ""}; // Error occurred
            }

            if (bytesRead == 0)
            {
                // Connection closed by server
                if (totalBytesRead == 0)
                    return {0, ""};
                break;
            }

            chunk[bytesRead] = '\0';
            message.append(chunk);
            totalBytesRead += bytesRead;

            // Check if we've received a complete message (ending with newline)
            if (message.find('\n') != string::npos)
            {
                break;
            }
        }

        // Remove the trailing newline if present
        if (!message.empty() && message.back() == '\n')
        {
            message.pop_back();
        }

        return {totalBytesRead, message};
    }

    ssize_t sendAll(const string &message)
    {
        const char *buffer = message.c_str();
        size_t totalLength = message.length();
        size_t bytesSent = 0;
        const size_t CHUNK_SIZE = 32;

        while (bytesSent < totalLength)
        {
            size_t remainingBytes = totalLength - bytesSent;
            size_t currentChunkSize = min(CHUNK_SIZE, remainingBytes);

            ssize_t result = write(sockfd, buffer + bytesSent, currentChunkSize);

            if (result < 0)
            {
                if (errno == EINTR)
                    continue; // Interrupted by signal, retry
                return -1;    // Error occurred
            }

            bytesSent += result;
        }

        return bytesSent;
    }

    // Replace existing receiveMessage function with:
    pair<ssize_t, string> recieveMessage()
    {
        return recvAll();
    }

    // Replace existing sendMessage function with:
    ssize_t sendMessage(string message)
    {
        if (!message.empty() && message.back() != '\n')
        {
            message += '\n'; // Add newline delimiter if not present
        }
        return sendAll(message);
    }
} clientObject;

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
            terminalObject.consoleStatement("Error in Reading");
        }
        else if (bytesRead == 0)
        {
            terminalObject.consoleStatement("Disconnecting from Server");
            break;
        }
        else
        {
            terminalObject.consoleStatement(message);
            if (message == "EXIT Processed")
            {
                break;
            }
        }
    }
    pthread_exit(NULL);
}

void *writeHandler(void *args)
{
    string message;
    while (true)
    {
        message = terminalObject.getInput();
        message += "\n";
        if (sizeof(message) > BUFFER_SIZE)
        {
            message[BUFFER_SIZE - 2] = '\n';
        }
        clientObject.sendMessage(message);
        message.pop_back();
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

    terminalObject.initNcurses();

    pthread_t readThread, writeThread;
    pthread_create(&readThread, NULL, readHandler, NULL);
    pthread_create(&writeThread, NULL, writeHandler, NULL);
    pthread_detach(writeThread);
    pthread_join(readThread, NULL);
    pthread_cancel(writeThread);

    clientObject.closeClient();
    terminalObject.closeTerminal();
    return 0;
}