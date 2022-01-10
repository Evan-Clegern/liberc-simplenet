#include "Networker-old.hpp"

using namespace ERCLIB::Net;

int main() {
	//Client sends first
	try {
		c_IPv4Addr Me(127,0,0,1);
		c_BaseSocket BaseUDP(Me, 8081, CONNECT_UDP);
		c_BaseSocket BaseStr(Me, 8081, CONNECT_TCP);
		
		c_IOSocket HandlerUDP(BaseUDP, DO_WAIT);
		c_IOSocket Handler(BaseStr, DO_WAIT, TCP_SETUP_CONNECT);
		c_IPv4Addr Them(127,0,0,2); u16 ThemPort = 0;
		
		char bufferOpen[5] = "Open";
		HandlerUDP.transmitUDP(bufferOpen, 5, Them, 8080);
		
		if (Handler.createConnectionTCP(Them, 8080)) {
			std::cout << "Connected.\n";
			char message[7] = "Hello";
			
			if (Handler.transmitTCP(message, 7, 1)) {
				std::cout << "Sent 1";
			} else {
				std::cout << "Not sent";
			}
			if (Handler.transmitTCP(message, 7, 1)) {
				std::cout << "Sent 2";
			} else {
				std::cout << "Not sent";
			}
			char message2[7] = "Later!";
			if (Handler.transmitTCP(message2, 7, 1)) {
				std::cout << "Sent 3";
			} else {
				std::cout << "Not sent";
			}
			Handler.copyPeerAddrTCP(&Them, &ThemPort);
			std::cout << Them.toText() << ':' << ThemPort;
		} else {
			std::cout << "What?";
		}
	} catch (c_NetError E) {
		std::cout << E.fullWhat();
	}
	return 0;
}
