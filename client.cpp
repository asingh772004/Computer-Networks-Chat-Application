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
    WINDOW *chatWin;
    WINDOW *inputWin;
    pthread_mutex_t consoleLock = PTHREAD_MUTEX_INITIALIZER;

    void initNcurses()
    {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);

        int chat_height = LINES - 3;
        chatWin = newwin(chat_height, COLS, 0, 0);
        inputWin = newwin(3, COLS, chat_height, 0);

        scrollok(chatWin, TRUE);
        box(inputWin, 0, 0);
        wrefresh(inputWin);
    }

    void closeTerminal()
    {
        int chat_height, chat_width;
        getmaxyx(chatWin, chat_height, chat_width);
        std::vector<std::string> chat_history;
        for (int i = 0; i < chat_height; i++)
        {
            char line[chat_width + 1];
            mvwinnstr(chatWin, i, 0, line, chat_width);
            line[chat_width] = '\0';
            chat_history.push_back(std::string(line));
        }
        wclear(inputWin);
        wrefresh(inputWin);
        delwin(inputWin);
        inputWin = nullptr;
        wresize(chatWin, LINES, COLS);
        mvwin(chatWin, 0, 0);
        wrefresh(chatWin);
        refresh();
        endwin();
        for (const auto &line : chat_history)
        {
            if (!line.empty() && line.find_first_not_of(' ') != std::string::npos)
            {
                std::cout << line << std::endl;
            }
        }
        std::cout << "\033[" << chat_height << "B";
        system("stty sane");
    }

    void consoleStatement(const string &message)
    {
        pthread_mutex_lock(&consoleLock);
        wprintw(chatWin, "%s\n", message.c_str());
        wrefresh(chatWin);
        pthread_mutex_unlock(&consoleLock);
    }

    string getInput()
    {
        string input;
        int ch;
        int max_width = COLS - 4;
        while (true)
        {
            ch = wgetch(inputWin);

            if (ch == '\n')
            {
                break;
            }
            if (ch == KEY_BACKSPACE || ch == 127)
            {
                if (!input.empty())
                {
                    input.pop_back();
                }
            }
            else if (ch != ERR && input.length() < max_width)
            {
                input += ch;
            }
            wclear(inputWin);
            box(inputWin, 0, 0);
            int center_y = 1;
            int start_x = 2;
            mvwprintw(inputWin, center_y, start_x, "%s", input.c_str());
            wrefresh(inputWin);
        }
        wclear(inputWin);
        box(inputWin, 0, 0);
        wrefresh(inputWin);
        consoleStatement("You: " + input);
        return input;
    }
} terminalObject;

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

    terminalObject.initNcurses();

    pthread_t readThread, writeThread;
    pthread_create(&readThread, NULL, readHandler, NULL);
    pthread_create(&writeThread, NULL, writeHandler, NULL);
    pthread_join(writeThread, NULL);
    pthread_cancel(readThread);

    clientObject.closeClient();
    terminalObject.closeTerminal();
    return 0;
}