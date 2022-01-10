#include "Networker Refined.hpp"

using namespace ERCLIB::Net;

int main() {
	try {
		c_IPv4Addr Srv(127,0,0,1);
		
		char updMsg[10] = "Link plz!";
		c_Socket_UDP ProbePort(18080, DO_WAIT);
		ProbePort.transmit(updMsg, 10, 0, Srv, 8081);
		ProbePort.shutdownSocket();
		
		c_Socket_TCP test(18080, DO_WAIT, TCP_SETUP_CONNECT);
		try {
			test.connectionEstablish(Srv, 8081);
		} catch (c_NetError B) {
			std::cout << "Couldn't connect (too soon?), Trying again ..\n";
			test.connectionEstablish(Srv, 8081);
		}
		
		char message[10] = "Newer API";
		test.transmit(message, 10, 0);
		if (test.receive(message, 10)) {
			std::cout << message;
		}
	} catch (c_NetError B) {
		std::cerr << B.fullWhat();
	}
}
