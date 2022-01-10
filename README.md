# liberc-simplenet
A relatively developer-friendly C++ API for the Linux Sockets system, implementing <unistd.h>, <sys/sockets.h>, <arpa/inet.h>, <sys/time.h>, and <fcntl.h> from the C library, with the C++ library and polymorphic classes. Provides two-way TCP and UDP sockets, as well as a specialty (but work in progress) "TCP Server" class, which enables multiple clients to connect to the same port on a server machine. It uses the C-based File Descriptor system to implement this without multithreading, and overall provides a useful interface for managing multiple inbound connections.

The files "client_man-simple.cpp" and "server_man-simple.cpp" illustrate a simple UDP "ping" and TCP session system with the API's basic socket system. Meanwhile, "client_man-adv.cpp" and "server_man-adv.cpp" illustrate the API's TCP Server system, with two sessions from a client and four sockets on the server.

## Warnings
This little API:
1. Started as an experiment.
2. Is not 100% stable.
3. Is only for Linux, as it uses the built-in C library.
4. <s>Blends C and C++ code within its namespace.</s> Current development version has isolated all non-macro objects to a subnamespace, "Clib", for cleanliness.

That in mind, it does seem to work on loopback interfaces, but I will test it on two different machines soon.

# Installation
Simply clone the git repository and do "make" to generate the main library. Use "make tests" to also compile the test code.
