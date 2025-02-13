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

map<int, string> Client_List;
map<string, int> Chat_Room;

typedef enum msgType
{
    BROADCAST,
    CONNECT,
    DISCONNECT,
    PRIVATE
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

char *commandHandler(string message, int sock_sender)
{
    string msg;
    msgType command;

    if (message[0] == '@')
    {
        command = PRIVATE;
    }
    else if (message.size() >= 7 && message.substr(0, 7) == "CONNECT")
    {
        command = CONNECT;
        msg = message.substr(7);
        msgParser(command, msg, sock_sender);
    }
    else if (message.size() >= 10 && message.substr(0, 10) == "DISCONNECT")
    {
        command = DISCONNECT;
        msg = message.substr(10);
        msgParser(command, msg, sock_sender);
    }
}