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

// Terminal UI (ncurses)
#include <ncurses.h> // For ncurses-based UI

using namespace std;

#define RESET "\033[0m"
#define YELLOW "\033[33m" // yellow
#define RED "\033[31m"    // Red color
#define GREEN "\033[32m"  // Green color
#define WHITE "\033[37m"  // White color

#define BUFFER_SIZE 256

class terminal
{
public:
    WINDOW *chat_win;
    WINDOW *input_win;
    pthread_mutex_t consoleLock = PTHREAD_MUTEX_INITIALIZER;

    void init_ncurses()
    {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);

        int chat_height = LINES - 3;
        chat_win = newwin(chat_height, COLS, 0, 0);
        input_win = newwin(3, COLS, chat_height, 0);

        scrollok(chat_win, TRUE);
        box(input_win, 0, 0);
        wrefresh(input_win);
    }

    void consoleStatement(const string &message)
    {
        pthread_mutex_lock(&consoleLock);
        wprintw(chat_win, "%s\n", message.c_str());
        wrefresh(chat_win);
        pthread_mutex_unlock(&consoleLock);
    }

    string getInput()
    {
        string input;
        int ch;
        while ((ch = wgetch(input_win)) != '\n')
        {
            if (ch == KEY_BACKSPACE || ch == 127)
            {
                if (!input.empty())
                {
                    input.pop_back();
                    int y, x;
                    getyx(input_win, y, x);
                    mvwaddch(input_win, y, x - 1, ' ');
                    wmove(input_win, y, x - 1);
                }
            }
            else
            {
                input += ch;
                waddch(input_win, ch);
            }
            wrefresh(input_win);
        }
        wclear(input_win);
        box(input_win, 0, 0);
        wrefresh(input_win);
        return input;
    }
} terminalObject;

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
            cout << RED << "Could not open Socket" << RESET << endl;
            exit(0);
        }
    }

    void getServer(char *argv[])
    {
        server = gethostbyname(argv[1]);
        if (server == NULL)
        {
            cout << RED << "Could not get Server" << RESET << endl;
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
            cout << RED << "Could not connect to Server" << RESET << endl;
            exit(0);
        }
    }

    void closeClient()
    {
        close(sockfd);
    }

    pair<ssize_t, string> recieveMessage()
    {
        bzero(buffer, 256);
        bytesRead = read(sockfd, buffer, sizeof(buffer));
        string message(buffer);
        return {bytesRead, message};
    }

    ssize_t sendMessage(string message)
    {
        char *msgPtr = &message[0];
        bytesSent = write(sockfd, msgPtr, sizeof(message));
        return bytesSent;
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
            terminalObject.consoleStatement("Server Disconnected");
            break;
        }
        else
        {
            terminalObject.consoleStatement(message);
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