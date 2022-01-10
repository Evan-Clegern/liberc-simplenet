#include "Networker Refined.hpp"

using namespace ERCLIB::Net;

int main() {
	try {
		c_IPv4Addr PartnerIP; u16 PartnerPort;
		c_Socket_UDP ProbePort(8081, DO_WAIT);
		char msgbuffer[10];
		ProbePort.receiveWithAddr(msgbuffer, 10, &PartnerIP, &PartnerPort);
		std::cout << "Received linkup request from " << PartnerIP.toText() << ':' << PartnerPort << '\n';
		ProbePort.shutdownSocket();
		
		c_Socket_TCP test(8081, DO_WAIT, TCP_SETUP_LISTEN);
		std::cout << "Waiting...\n";
		test.connectionAccept();
		std::cout << "Connected.\n";
		
		if (test.receive(msgbuffer, 10)) {
			std::cout << msgbuffer;
			
			char msg2[10] = "Goodbye..";
			test.transmit(msg2, 10, 1);
		}
	} catch (c_NetError B) {
		std::cerr << B.fullWhat();
	}
}
