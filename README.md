# liberc-simplenet
A simple C++ extension of the integrated Linux "sys/sockets.h" system, with error messages and general polish. There is still some more cleanup and documentation to be done, but it does work (at least on loopback, with different ports).


The files "client_man.cpp" and "server_man.cpp" illustrate a simple UDP "ping" and TCP session system with the library.


## Refined version
So yeah, a day after making the first one, I decided it needed polish (I mean, two classes is kidna gross for the functionality of one).

So now it uses C++'s polymorphism a little, and has some more general utility tied in. Seems to work a little better from what I tested, but since it doesn't come with a few "wait" commands, I had to make a quick jerry-rig to ensure the TCP connection (in the example file) can be established. Other than that, it still seems to work smoothly, and the code itself looks a bit nicer now.
