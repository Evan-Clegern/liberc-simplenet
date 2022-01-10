#include "Networker Refined.hpp"

using namespace ERCLIB::Net;

int main() {
	try {
		c_Socket_TCP test(1802, DO_WAIT, TCP_SETUP_CONNECT);
		c_IPv4Addr them(127,0,0,1);
		test.connectionEstablish(them, 8100);
		c_Socket_TCP test2(1801, DO_WAIT, TCP_SETUP_CONNECT);
		test2.connectionEstablish(them, 8100);
		
		char mesg[10] = "Shutdown!", mesg2[12] = "No shutdown";
		test.transmit(mesg, 10, 1);
		test2.transmit(mesg2, 12, 0);
		test.shutdownSocket();
		Clib::sleep(1);
		test2.transmit(mesg, 10, 0);
		test2.shutdownSocket();
	} catch (std::exception B) {
		std::cerr << B.what();
	}
}
