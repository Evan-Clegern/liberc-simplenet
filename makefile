liberc-networker.so:
	g++ -std=c++17 -c "Networker Refined.cpp" -O2 -fPIC -o Networker.o
	g++ -std=c++17 -shared -o liberc-networker.so Networker.o -O2 -fPIC
	rm Networker.o


.PHONY: tests

tests: liberc-networker.so
	g++ -std=c++17 -L. -I. -o server_man-simple server_man-simple.cpp -lerc-networker -Wl,-rpath=.
	g++ -std=c++17 -L. -I. -o client_man-simple client_man-simple.cpp -lerc-networker -Wl,-rpath=.

	g++ -std=c++17 -L. -I. -o server_man-adv server_man-adv.cpp -lerc-networker -Wl,-rpath=.
	g++ -std=c++17 -L. -I. -o client_man-adv client_man-adv.cpp -lerc-networker -Wl,-rpath=.
