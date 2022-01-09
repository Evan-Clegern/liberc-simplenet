#include "Networker.hpp"

using namespace ERCLIB::Net;

int main() {
	//Server receives first
	try {
		c_IPv4Addr Me(127,0,0,2);
		c_BaseSocket BaseProbe(Me, 8080, CONNECT_UDP);
		c_BaseSocket BaseOpen(Me, 8080, CONNECT_TCP);
		
		c_IOSocket HandlerOpen(BaseOpen, DO_WAIT, TCP_SETUP_LISTEN);
		c_IOSocket HandlerProbe(BaseProbe, DO_WAIT);
		
		c_IPv4Addr Them; u16 ThemPort = 0;
		
		char bufferProbe[5] = "****";
		HandlerProbe.receiveAddrUDP(bufferProbe, 5, &Them, &ThemPort);
		
		std::cout << Them.toText() << ':' << ThemPort << '\n';
		
		if (bufferProbe != "****") {
			
			HandlerOpen.acceptConnectionTCP();
			std::cout << "Accepted.\n" << std::flush;
			char message[7];

			HandlerOpen.copyPeerAddrTCP(&Them, &ThemPort);
			HandlerOpen.receive(message, 7);
			std::cout << message << std::flush;
			HandlerOpen.receive(message, 7);
			std::cout << message << std::flush;
			HandlerOpen.receive(message, 7);
			std::cout << message << '\n' << Them.toText() << ':' << ThemPort;
		} else {
			std::cout << "err..";
		}
		
	} catch (c_NetError E) {
		std::cout << E.fullWhat();
	}
	return 0;
}
