# Overview

Simple project to get used to using the sockets API.

This software is a basic chatroom. It has a server and serperate client program. The server can support multiple clients. The server's main job is to relay messages from one client to all the other clients.
It also supports the sending of ANS files that display themselves to other clients.

I made this to help better my understanding and get practice in on network programming.

[Software Demo Video](http://youtube.com)

# Network Communication

This software uses a client/server model

The sockets used are TCP sockets and the port is set to 50000, but it can be redefined in the header file.

Main communications between the client and server is simple messages typed by the user of the client. It communicates a name approval system and sending ANS files.

# Development Environment

Vim for editing and gcc for compiling

C89 was used along with the POSIX socket api and POSIX threads.

To build the server:
`gcc -std=c89 -pthread server.c -o server`

and the client:
`gcc -std=c89 -pthread client.c -o client`

# Useful Websites

* [man(2)](https://man7.org/linux/man-pages/dir_section_2.html)
* [Stack Overflow](https://stackoverflow.com/)

# Future Work

* Improve the client's use of the terminal
* Add authetication for users
* Add user privileges