# Computer_Networks_Chat_Application

<br>

## Overview

This is a multi-client chat application implemented in C++ using a server-client model. The server supports multiple clients and allows broadcasting, private messaging, and client management features using POSIX sockets and multithreading.

<br>

## Features

#### Multi client support:
* Supports up to 5 simultaneous clients (configurable via MAX_CLIENTS)
* Extra clients are automatically closed

#### Non-blocking I/O with select():
* Uses select() to manage new connections and client communications, eliminating the need for multithreading on the server side

#### Client Alias Management:
* Each client must set an alias. If an alias is already taken, the server prompts the client for another alias.

#### Chat Room Join/Leave Mechanism
* Clients must explicitly join the chat room using the CONNECT command
* Users can send broadcast or private messages in the chat room

#### Broadcast Messaging:
* Messages sent without a command are broadcast to all clients in the chat room

#### Private Messaging:
* Prefix a message with @username to send a private message to one or more specific users
* The sender receives a notificationIf the recipient is not found

#### DISCONNECT and EXIT:
* Commands to leave the chat room or close the connection respectively

#### Customized terminal:
* ncurses library used for custom terminal with seperate chat and input window, along with scroll option in chat
* All client terminal related functions are included in terminal.h header file
* Collission avoided between inputs and outputs due to asynchronous behaviour

#### Multithreading:
* Threaded client handling in server for scalability and asynchronous inputs and outputs
* Threaded input and output handlers in client for ansynchronity

<br>

## Technologies Used

* C++ (Standard Library, POSIX APIs)
* Sockets (TCP/IP)
* pthreads for multithreading
* ncurses library for customized terminal
* ANSI escape sequences for colored terminal output

<br>

## Compilation

#### Compile the server
```g++ -o server server.cpp -lpthread```

#### Compile the client
```g++ -o client client.cpp -lpthread -lncurses```
*make sure client.cpp and terminal.h are in the same directory*

<br>

## USAGE

### Starting the server

#### Run the server with a specified port number:
```./server <port_number>```
#### Example:
```./server 4761```

### Connecting clients
#### Run the client and specify the server IP and port:
```./client <server_ip> <port_number>```
#### Example:
```./client 127.0.0.1 4761```

<br>

### Commands
|Command|Description|
|---|---|
|CONNECT|Connects the user to the chatroom|
|DISCONNECT|Disconnects the user from the chatroom|
|EXIT|Exits the chat application|
|@username \<message\>|Sends a private message to a user|
|\<message\>|Broadcasts a message to all connected users except the sender|

<br>
<br>

## SAMPLE IMAGES


#### Server
![serverImage](https://github.com/user-attachments/assets/cb52b6ce-da31-4757-b52d-a01b7053218b)

#### First Client
![client1Image](https://github.com/user-attachments/assets/55430f41-2ee3-4481-b907-0fbe623e1611)


#### Second Client
![client1Image](https://github.com/user-attachments/assets/25dccb81-9ef9-4c26-954a-0935a93bb95d)

#### Third Client
![client3Image](https://github.com/user-attachments/assets/433fe844-e4a3-4dc3-9a2f-3b461a1deac1)





