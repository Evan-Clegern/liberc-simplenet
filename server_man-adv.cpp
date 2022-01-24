#include "Networker Refined.hpp"

using namespace ERCLIB::Net;

int main() {
	try {
		c_TCP_Server testServer(8100, 4); //Four connections on port 8100
		
		while (true) {
			std::vector<short> vals = testServer.singleLoopOp();
			
			if (vals.size() > 0) {
				for (short i : vals) {
					if (i < 0) {
						std::cout << "New client!\n";
					} else {
						std::cout << "Receiving from client #" << i << '\n';
						char mbuf[32];
						if (testServer.receiveOnSubsock(i, mbuf, 32)) {
							if (std::string(mbuf) == "Shutdown!") {
								std::cout << "Client requested shutdown!\n";
								testServer.shutdownSubsock(i);
							} else {
								std::cout << "Client Message: " << mbuf << '\n';
							}
						}
						std::cout << std::flush;
					}
				}
			} else {
				Clib::usleep(250000); //0.25s
			}
			Clib::usleep(2);
		}
	} catch (std::exception* B) {
		std::cerr << B->what();
		return 1;
	}
	return 0;
}
