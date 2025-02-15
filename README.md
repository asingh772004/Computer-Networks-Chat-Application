# Computer_Networks_Chat_Application

Overview

This is a multi-client chat application implemented in C++ using a server-client model. The server supports multiple clients and allows broadcasting, private messaging, and client management features using POSIX sockets and multithreading.

Features

* Multi-client support (up to 5 clients at a time)
* Private messaging using @username prefix
* Broadcast messaging
* User alias management
* Connection and disconnection notifications
* Threaded client handling for scalability

  Technologies Used

* C++ (Standard Library, POSIX APIs)
* Sockets (TCP/IP)
* pthreads for multithreading
* ANSI escape sequences for colored terminal output
