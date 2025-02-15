# Computer_Networks_Chat_Application

## Overview

This is a multi-client chat application implemented in C++ using a server-client model. The server supports multiple clients and allows broadcasting, private messaging, and client management features using POSIX sockets and multithreading.

## Features

* Multi-client support (up to 5 clients at a time)
* Private messaging using @username prefix
* Broadcast messaging
* User alias management
* Connection and disconnection notifications
* Threaded client handling for scalability

## Technologies Used

* C++ (Standard Library, POSIX APIs)
* Sockets (TCP/IP)
* pthreads for multithreading
* ANSI escape sequences for colored terminal output

## Compilation

  ### Compile the server
```g++ -o server server.cpp -lpthread```

  ### Compile the client
```g++ -o client client.cpp -lpthread -lncurses```

## USAGE
  ### Starting the server

  #### Run the server with a specified port number:
```./server <port_number>```
  #### Example:
```./server 8080```

### Connecting clients
#### Run the client and specify the server IP and port:
```./client <server_ip> <port_number>```
#### Example:
```./client 127.0.0.1 8080```

### Commands
|Command|Description|
|---|---|
|CONNECT|Connects the user to the chatroom|
|DISCONNECT|Disconnects the user from the chatroom|
|EXIT|Exits the chat application|
|@username \<message\>|Sends a private message to a user|
|\<message\>|Broadcasts a message to all connected users except the sender|
