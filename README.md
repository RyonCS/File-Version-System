## Title:
 
Server-based Versioning File Storage System

## Overview:

For this project, I utilized C to create a server and a client that can speak to each other and transmit files. The client connects to the server via socket-programming and issues command line arguments to send, retrieve, or delete files on the server.The server stores files and the version history. If the server receives a write command from the client, it stores the file in the provided directory. If the file sent has the same name as an already stored file, a copy is made and a version history is kept. 

## How to Build and Compile
Program can be built from the make file using: make all.
Client and server programs need to be run in different windows.
Server: ./server
Client: ./rfs (with commands such as WRITE "file name").
