# liberc-simplenet
A relatively developer-friendly C++ API for the Linux Sockets system, implementing <unistd.h>, <sys/sockets.h>, <arpa/inet.h>, <sys/time.h>, and <fcntl.h> from the C library, with the C++ library and polymorphic classes. Provides two-way TCP and UDP sockets, as well as a specialty (but work in progress) "TCP Server" class, which enables multiple clients to connect to the same port on a server machine. It uses the C-based File Descriptor system to implement this without multithreading, and overall provides a useful interface for managing multiple inbound connections.

The files "client_man.cpp" and "server_man.cpp" illustrate a simple UDP "ping" and TCP session system with the API's basic socket system.


## Refined version
So yeah, a day after making the first one, I decided it needed polish (I mean, two classes is kidna gross for the functionality of one).

So now it uses C++'s polymorphism a little, and has some more general utility tied in. Seems to work a little better from what I tested, but since it doesn't come with a few "wait" commands, I had to make a quick jerry-rig to ensure the TCP connection (in the example file) can be established. Other than that, it still seems to work smoothly, and the code itself looks a bit nicer now.
