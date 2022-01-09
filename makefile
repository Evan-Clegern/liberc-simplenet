.PHONY: all

all: liberc-networker.so
	g++ -std=c++17 -L. -I. -o server_man server_man.cpp -lerc-networker -Wl,-rpath=.
	g++ -std=c++17 -L. -I. -o client_man client_man.cpp -lerc-networker -Wl,-rpath=.

liberc-networker.so:
	g++ -std=c++17 -c Networker.cpp -O2 -fPIC
	g++ -std=c++17 -shared -o liberc-networker.so Networker.o -O2 -fPIC
	rm Networker.o
