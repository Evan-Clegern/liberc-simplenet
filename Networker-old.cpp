#include "Networker.hpp"

namespace ERCLIB { namespace Net {

std::string errLvlTxt(ErrorLevel level) {
	switch (level) {
		case ERR_MIN:
			return "MIN";
		case ERR_LOW:
			return "LOW";
		case ERR_MED:
			return "MED";
		case ERR_SEV:
			return "SEV";
		default:
			return "FTL";
	};
}

c_NetError::c_NetError(const char* text, const ErrorLevel level) {
	m_about = std::string(text);
	m_severity = level;
}

c_NetError::c_NetError(const std::string text, const ErrorLevel level) {
	m_about = text;
	m_severity = level;
}

const std::string c_NetError::what() const noexcept {
	return this->m_about;
}

const ErrorLevel c_NetError::getLevel() const noexcept {
	return this->m_severity;
}

const std::string c_NetError::fullWhat() const noexcept {
	std::string N = '[' + errLvlTxt(this->m_severity) + "] " + this->m_about;
	return N;
}


c_IPv4Addr::c_IPv4Addr() {
	auto N = INADDR_ANY;
	A = (N >> 24);
	B = (N >> 16);
	C = (N >> 8);
	D = u8(N);
}
void c_IPv4Addr::operator=(const c_IPv4Addr& b) {
	this->A = b.A;
	this->B = b.B;
	this->C = b.C;
	this->D = b.D;
}

const AddressClass c_IPv4Addr::getClass() const noexcept {
	if (this->A <= 127) return CLASS_A;
	if (this->A == 127) return LOOPBACK;
	if (this->A <= 191) return CLASS_B;
	if (this->A <= 223) return CLASS_C;
	if (this->A <= 239) return MULTICAST;
	return EXPERIMENT;
}

const bool c_IPv4Addr::isPrivate() const noexcept {
	if (this->A == 10) return true;
	if ((this->A == 169) && (this->B==254)) return true;
	if ((this->A == 172) && (this->B<=31) && (this->B >= 16)) return true;
	if ((this->A == 192) && (this->B==168)) return true;
	return false;
}

const bool c_IPv4Addr::isAPIPA() const noexcept {
	if ((this->A == 169) && (this->B==254)) return true;
	return false;
}

const bool c_IPv4Addr::isLocalLoopback() const noexcept {
	return (this->A == 127) && (this->B == 0) && (this->C == 0) && (this->D == 1);
}

const u32 c_IPv4Addr::toUint() const noexcept {
	u32 Bob = this->A;
	Bob <<= 8; Bob += this->B;
	Bob <<= 8; Bob += this->C;
	Bob <<= 8; Bob += this->D;
	return Bob;
}

const std::string c_IPv4Addr::toText() const noexcept {
	std::string N = std::to_string(short(this->A));
	N += '.'; N += std::to_string(short(this->B));
	N += '.'; N += std::to_string(short(this->C));
	N += '.'; N += std::to_string(short(this->D));
	return N;
}

const sockaddr_in c_IPv4Addr::makeCSocket(u16 i_port) const noexcept {
	sockaddr_in N;
	N.sin_family = AF_INET;
	N.sin_addr.s_addr = htonl(this->toUint());
	N.sin_port = htons(i_port);
	return N;
}

c_BaseSocket::c_BaseSocket(c_IPv4Addr i_addr, u16 i_port, ConnType i_type) : m_address(i_addr) {
	m_socketPort = i_port;
	m_sockType = i_type;
	if (i_type) {
		m_socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
		if (m_socketDescriptor <= 0) {
			if (errno == EAFNOSUPPORT) {
				throw c_NetError("Unable to establish TCP socket -- Address Not Supported!", ERR_FTL);
			} else if ((errno == EMFILE) || (errno == ENFILE)) {
				throw c_NetError("Unable to establish TCP socket -- Open Descriptors Exhausted!", ERR_FTL);
			} else if (errno == EPROTONOSUPPORT) {
				throw c_NetError("Unable to establish TCP socket -- Protocol Implementation Not Supported!", ERR_FTL);
			} else if (errno == EPROTOTYPE) {
				throw c_NetError("Unable to establish TCP socket -- Protocol Socket Not Supported!", ERR_FTL);
			} else if (errno == EACCES) {
				throw c_NetError("Unable to establish TCP socket -- Access Deined!", ERR_FTL);
			} else if ((errno == ENOBUFS) || (errno == ENOMEM) || (errno == ENOSR)) {
				throw c_NetError("Unable to establish TCP socket -- Insufficient Resources!", ERR_FTL);
			} else {
				throw c_NetError("Unable to enable TCP socket -- Unknown Error (" + std::to_string(errno) + ')', ERR_FTL);
			}
		}
	} else {
		m_socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_socketDescriptor <= 0) {
			if (errno == EAFNOSUPPORT) {
				throw c_NetError("Unable to establish UDP socket -- Address Not Supported!", ERR_FTL);
			} else if ((errno == EMFILE) || (errno == ENFILE)) {
				throw c_NetError("Unable to establish UDP socket -- Open Descriptors Exhausted!", ERR_FTL);
			} else if (errno == EPROTONOSUPPORT) {
				throw c_NetError("Unable to establish UDP socket -- Protocol Implementation Not Supported!", ERR_FTL);
			} else if (errno == EPROTOTYPE) {
				throw c_NetError("Unable to establish UDP socket -- Protocol Socket Not Supported!", ERR_FTL);
			} else if (errno == EACCES) {
				throw c_NetError("Unable to establish UDP socket -- Access Deined!", ERR_FTL);
			} else if ((errno == ENOBUFS) || (errno == ENOMEM) || (errno == ENOSR)) {
				throw c_NetError("Unable to establish UDP socket -- Insufficient Resources!", ERR_FTL);
			} else {
				throw c_NetError("Unable to enable UDP socket -- Unknown Error (" + std::to_string(errno) + ')', ERR_FTL);
			}
		}
	}
	int tempopt = 1;
	int X = setsockopt(m_socketDescriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_KEEPALIVE, &tempopt, sizeof(tempopt));
	if (X != 0){
		if (errno == EBADF) {
			throw c_NetError("Unable to configure unbound socket -- Unknown Socket Error!", ERR_FTL);
		} else if (errno == EDOM) {
			throw c_NetError("Unable to configure unbound socket -- Timeout Domain Error!", ERR_FTL);
		} else if (errno == EFAULT) {
			throw c_NetError("Unable to configure unbound socket -- Invalid Option Pointer!", ERR_FTL);
		} else if (errno == EINVAL) {
			throw c_NetError("Unable to configure unbound socket -- Invalid Options!", ERR_FTL);
		} else if (errno == EISCONN) {
			throw c_NetError("Unable to configure unbound socket -- Socket In Use!", ERR_FTL);
		} else if (errno == ENOPROTOOPT) {
			throw c_NetError("Unable to configure unbound socket -- Protocol Options Not Supported!", ERR_FTL);
		} else if (errno == ENOTSOCK) {
			throw c_NetError("Unable to configure unbound socket -- Descriptor is not a Socket!", ERR_FTL);
		} else {
			throw c_NetError("Unable to configure unbound socket -- Unknown Failure (" + std::to_string(errno) + ')', ERR_FTL);
		}
	}
	m_cSocket = i_addr.makeCSocket(i_port);
	X = bind(m_socketDescriptor, (sockaddr*)&(m_cSocket), sizeof(m_cSocket));
	if (X != 0) {
		if (errno == EADDRINUSE) {
			throw c_NetError("Unable to bind socket -- Specified Address In-Use!", ERR_MED);
		} else if (errno == EADDRNOTAVAIL) {
			throw c_NetError("Unable to bind socket -- Address Not Available!", ERR_MED);
		} else if (errno == EAFNOSUPPORT) {
			throw c_NetError("Unable to bind socket -- Address Unsupported!", ERR_MED);
		} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
			throw c_NetError("Unable to bind socket -- Descriptor is not a Socket!", ERR_MED);
		} else if (errno == EINVAL) {
			throw c_NetError("Unable to bind socket -- Socket Invalid or Shutdown!", ERR_MED);
		} else if (errno == EOPNOTSUPP) {
			throw c_NetError("Unable to bind socket -- Socket Type Cannot be Bound!", ERR_MED);
		} else {
			throw c_NetError("Unable to bind socket -- Unknown Failure (" + std::to_string(errno) + ')', ERR_MED);
		}
	}
	m_bound = 1;
}

c_BaseSocket::~c_BaseSocket() {
	shutdown(this->m_socketDescriptor, SHUT_RDWR);
}


c_IOSocket::c_IOSocket(c_BaseSocket& i_sock, ConnWait i_connectWait, TCPSetup i_TCPMode) : m_Socket(i_sock) {
	if (i_sock.inUse) throw c_NetError("Base socket is in use!", ERR_SEV);
	i_sock.inUse = 1;
	m_waitForCnt = i_connectWait;
	m_TCP = i_sock.m_sockType;
	if (m_TCP && (i_TCPMode == TCP_SETUP_LISTEN)) {
		if (!i_sock.m_bound) {
			throw c_NetError("Unable to open bound socket for listening -- Socket Is Unbound!", ERR_LOW);
		}
		int X = listen(i_sock.m_socketDescriptor, 2); //Integer is backlog level for outstanding connections
		if (X != 0) {
			if ((errno == EBADF) && (errno == ENOTSOCK)) {
				throw c_NetError("Unable to open bound socket for listening -- Socket Invalid!", ERR_LOW);
			} else if (errno == EDESTADDRREQ) {
				throw c_NetError("Unable to open bound socket for listening -- Socket Not Bound!", ERR_LOW);
			} else if (errno == EINVAL) {
				throw c_NetError("Unable to open bound socket for listening -- Socket Busy!", ERR_LOW);
			} else if (errno == EOPNOTSUPP) {
				throw c_NetError("Unable to open bound socket for listening -- Socket Protocol Listen Error!", ERR_LOW);
			} else {
				throw c_NetError("Unable to open bound socket for listening -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
			}
		}
		if (!i_connectWait) {
			int flags = fcntl(m_Socket.m_socketDescriptor, F_GETFL, 0);
			m_oldFlags = flags;
			fcntl(m_Socket.m_socketDescriptor, F_SETFL, flags | O_NONBLOCK);
		}
		m_TCPInp = TCP_SETUP_LISTEN;
	} else if (m_TCP) {
		//If we're just connecting, it'll use whatever. Better if we bind it.
		m_TCPInp = TCP_SETUP_CONNECT;
		if (!i_connectWait) {
			int flags = fcntl(m_Socket.m_socketDescriptor, F_GETFL, 0);
			m_oldFlags = flags;
			fcntl(m_Socket.m_socketDescriptor, F_SETFL, flags | O_NONBLOCK);
		}
	} else {
		if (!i_connectWait) {
			int flags = fcntl(m_Socket.m_socketDescriptor, F_GETFL, 0);
			m_oldFlags = flags;
			fcntl(m_Socket.m_socketDescriptor, F_SETFL, flags | O_NONBLOCK);
		}
		m_operateSocket = m_Socket.m_socketDescriptor;
	}
}

c_IOSocket::~c_IOSocket() {
	if (!this->m_waitForCnt) {
		fcntl(this->m_Socket.m_socketDescriptor, F_SETFL, this->m_oldFlags);
	}
	this->m_Socket.inUse = 0;
	if (this->m_socketized) {
		shutdown(this->m_operateSocket, SHUT_RDWR);
	}
}

//Returns TRUE if connection established, FALSE if not.
bool c_IOSocket::acceptConnectionTCP() {
	if (this->m_TCPInp == TCP_SETUP_CONNECT) {
		throw c_NetError("Unable to accept connection -- IOSocket not in TCP Listening mode!", ERR_LOW);
	}
	int lene = sizeof(sockaddr);
	this->m_operateSocket = accept(this->m_Socket.m_socketDescriptor, (sockaddr*)&this->m_Socket.m_cSocket, (socklen_t*)&lene);
	if (this->m_operateSocket <= 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
			return false;
		} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
			throw c_NetError("Unable to accept connection -- Socket Invalid!", ERR_LOW);
		} else if (errno == ECONNABORTED) {
			throw c_NetError("Unable to accept connection -- Connection Aborted!", ERR_LOW);
		} else if (errno == ECONNREFUSED) {
			throw c_NetError("Unable to accept connection -- Connection Refused!", ERR_LOW);
		} else if (errno == EINTR) {
			throw c_NetError("Unable to accept connection -- Operation Interrupted!", ERR_LOW);
		} else if ((errno == EMFILE) || (errno == ENFILE)) {
			throw c_NetError("Unable to accept connection -- Too Many Active Connections!", ERR_LOW);
		} else if (errno == EOPNOTSUPP) {
			throw c_NetError("Unable to accept connection -- Socket Type Refused!", ERR_LOW);
		} else {
			throw c_NetError("Unable to accept connection -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
		}
	}
	this->m_socketized = 1;
	return true;
}
//Also returns TRUE/FALSE
bool c_IOSocket::createConnectionTCP(sockaddr_in i_client) {
	if (this->m_TCPInp == TCP_SETUP_LISTEN) {
		throw c_NetError("Unable to establish connection -- IOSocket in TCP Listening mode!", ERR_LOW);
	}
	int X = connect(this->m_Socket.m_socketDescriptor, (sockaddr*)&i_client, sizeof(sockaddr));
	if (X != 0) {
		if (errno == EADDRNOTAVAIL) {
			throw c_NetError("Unable to establish connection -- Address Unavailable!", ERR_MED);
		} else if (errno == EAFNOSUPPORT) {
			throw c_NetError("Unable to establish connection -- Address Family Invalid!", ERR_LOW);
		} else if (errno == EALREADY) {
			throw c_NetError("Unable to establish connection -- Request In Progress!", ERR_MIN);
		} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
			throw c_NetError("Unable to establish connection -- Socket Invalid!", ERR_LOW);
		} else if (errno == ECONNREFUSED) {
			throw c_NetError("Unable to establish connection -- Connection Refused (Port is likely not listening)!", ERR_LOW);
		} else if (errno == EFAULT) {
			throw c_NetError("Unable to establish connection -- Cannot Access Address!", ERR_LOW);
		} else if (errno == EINPROGRESS) {
			return 0; //Establishing Asychronously
		} else if (errno == EINTR) {
			throw c_NetError("Unable to establish connection -- Operation Interrupted!", ERR_LOW);
		} else if (errno == EISCONN) {
			return 1; //Already connected
		} else if (errno == ENETUNREACH) {
			throw c_NetError("Unable to establish connection -- Destination Unreachable!", ERR_MED);
		} else if (errno == EPROTOTYPE) {
			throw c_NetError("Unable to establish connection -- Bad Port Type!", ERR_LOW);
		} else if (errno == ETIMEDOUT) {
			throw c_NetError("Unable to establish connection -- Timed Out!", ERR_LOW);
		} else {
			throw c_NetError("Unable to establish connection -- Unknown Failure (" + std::to_string(errno) + ')', ERR_MED);
		}
	}
	this->m_operateSocket = this->m_Socket.m_socketDescriptor;
	this->m_socketized = 1;
	return 1;
}
bool c_IOSocket::createConnectionTCP(c_IPv4Addr i_client, u16 i_port) {
	if (this->m_TCPInp == TCP_SETUP_LISTEN) {
		throw c_NetError("Unable to establish connection -- IOSocket in TCP Listening mode!", ERR_LOW);
	}
	sockaddr_in N = i_client.makeCSocket(i_port);
	int X = connect(this->m_Socket.m_socketDescriptor, (sockaddr*)&N, sizeof(sockaddr));
	if (X != 0) {
		if (errno == EADDRNOTAVAIL) {
			throw c_NetError("Unable to establish connection -- Address Unavailable!", ERR_MED);
		} else if (errno == EAFNOSUPPORT) {
			throw c_NetError("Unable to establish connection -- Address Family Invalid!", ERR_LOW);
		} else if (errno == EALREADY) {
			throw c_NetError("Unable to establish connection -- Request In Progress!", ERR_MIN);
		} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
			throw c_NetError("Unable to establish connection -- Socket Invalid!", ERR_LOW);
		} else if (errno == ECONNREFUSED) {
			throw c_NetError("Unable to establish connection -- Connection Refused (Port is likely not listening)!", ERR_LOW);
		} else if (errno == EFAULT) {
			throw c_NetError("Unable to establish connection -- Cannot Access Address!", ERR_LOW);
		} else if (errno == EINPROGRESS) {
			return 0; //Establishing Asychronously
		} else if (errno == EINTR) {
			throw c_NetError("Unable to establish connection -- Operation Interrupted!", ERR_LOW);
		} else if (errno == ENETUNREACH) {
			throw c_NetError("Unable to establish connection -- Destination Unreachable!", ERR_MED);
		} else if (errno == EPROTOTYPE) {
			throw c_NetError("Unable to establish connection -- Bad Port Type!", ERR_LOW);
		} else if (errno == ETIMEDOUT) {
			throw c_NetError("Unable to establish connection -- Timed Out!", ERR_LOW);
		} else {
			throw c_NetError("Unable to establish connection -- Unknown Failure (" + std::to_string(errno) + ')', ERR_MED);
		}
	}
	this->m_operateSocket = this->m_Socket.m_socketDescriptor;
	this->m_socketized = 1;
	return 1;
}

bool c_IOSocket::copyPeerAddrTCP(c_IPv4Addr* ip_storeMain, u16* ip_storePort) {
	if (!this->m_socketized) return 0;
	sockaddr_in bruh; unsigned int bruhSize = sizeof(bruh);
	
	int gval = getpeername(this->m_operateSocket, (sockaddr*)&bruh, &bruhSize);
	if (gval < 0) {
		if ((errno == EBADF) || (errno == ENOTSOCK)) {
			throw c_NetError("Unable to get peer data -- Socket Invalid!", ERR_MED);
		} else if (errno == EFAULT) {
			throw c_NetError("Unable to get peer data -- Cannot Write to Inputs!", ERR_LOW);
		} else if (errno == EINVAL) {
			throw c_NetError("Unable to get peer data -- Socket Shut Down!", ERR_LOW);
		} else if (errno == ENOTCONN) {
			throw c_NetError("Unable to get peer data -- Not Connected!", ERR_MED);
		} else {
			throw c_NetError("Unable to get peer data -- Unknwon Failure (" + std::to_string(errno) + ')', ERR_LOW);	
		}
	}
	*ip_storePort = ntohs(bruh.sin_port);
	u32 B = ntohl(bruh.sin_family);
	u8 nA = u8(B >> 24), nB = u8(B >> 16), nC = u8(B >> 8), nD = u8(B);
	c_IPv4Addr newer(nA, nB, nC, nD);
	*ip_storeMain = newer;
	return 1;
}

//Returns TRUE if anything was copied, FALSE if not.
bool c_IOSocket::receive(char* i_bufferItem, u32 i_buffSize) {
	if (!this->m_socketized) {
		if (this->m_Socket.m_sockType) return 0; //TCP needs to be connected
		//UDP
		auto readData = recv(this->m_operateSocket, i_bufferItem, i_buffSize, MSG_WAITALL); //Even tho not in block mode by default, at least wait until it's all ready
		if (readData == 0) return 0;
		if (readData < 0) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				return false; //We aren't waiting for a packet
			} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
				throw c_NetError("Unable to receive listen data -- Socket Invalid!", ERR_LOW);
			} else if (errno == ECONNABORTED) {
				throw c_NetError("Unable to receive listen data -- Connection Aborted!", ERR_LOW);
			} else if (errno == ECONNREFUSED) {
				throw c_NetError("Unable to receive listen data -- Connection Refused!", ERR_LOW);
			} else if (errno == ECONNRESET) {
				throw c_NetError("Unable to receive listen data -- Connection Reset!", ERR_LOW);
			} else if (errno == EINTR) {
				throw c_NetError("Unable to receive listen data -- Operation Interrupted!", ERR_LOW);
			} else if (errno == EOPNOTSUPP) {
				throw c_NetError("Unable to receive listen data -- Socket Type Refused!", ERR_LOW);
			} else if (errno == ETIMEDOUT) {
				throw c_NetError("Unable to receive listen data -- Connection Timed Out!", ERR_LOW);
			} else if (errno == EFAULT) {
				throw c_NetError("Unable to receive listen data -- Invalid Buffer!", ERR_LOW);
			} else {
				throw c_NetError("Unable to receive listen data -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
			}
		}
	} else {
		auto readData = recv(this->m_operateSocket, i_bufferItem, i_buffSize, MSG_WAITALL); //Even tho not in block mode by default, at least wait until it's all ready
		if (readData == 0) return 0;
		if (readData < 0) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				return false; //We aren't waiting for a packet
			} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
				throw c_NetError("Unable to receive listen data -- Socket Invalid!", ERR_LOW);
			} else if (errno == ECONNABORTED) {
				throw c_NetError("Unable to receive listen data -- Connection Aborted!", ERR_LOW);
			} else if (errno == ECONNREFUSED) {
				throw c_NetError("Unable to receive listen data -- Connection Refused!", ERR_LOW);
			} else if (errno == ECONNRESET) {
				throw c_NetError("Unable to receive listen data -- Connection Reset!", ERR_LOW);
			} else if (errno == EINTR) {
				throw c_NetError("Unable to receive listen data -- Operation Interrupted!", ERR_LOW);
			} else if (errno == EOPNOTSUPP) {
				throw c_NetError("Unable to receive listen data -- Socket Type Refused!", ERR_LOW);
			} else if (errno == ETIMEDOUT) {
				throw c_NetError("Unable to receive listen data -- Connection Timed Out!", ERR_LOW);
			} else if (errno == EFAULT) {
				throw c_NetError("Unable to receive listen data -- Invalid Buffer!", ERR_LOW);
			} else {
				throw c_NetError("Unable to receive listen data -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
			}
		}
	}
	return 1;
}

bool c_IOSocket::receiveAddrUDP(char* i_bufferItem, u32 i_buffSize, c_IPv4Addr* ip_storeDataFrom, u16* ip_storePortFrom) {
	sockaddr_in bruh; unsigned int bruhSize = sizeof(bruh);
	if (!this->m_socketized) {
		if (this->m_Socket.m_sockType == CONNECT_TCP) return 0; //TCP needs to be connected
		//UDP
		auto readData = recvfrom(this->m_operateSocket, i_bufferItem, i_buffSize, MSG_WAITALL, (sockaddr*)&bruh, &bruhSize);
		if (readData == 0) return 0;
		if (readData < 0) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				return false; //We aren't waiting for a packet
			} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
				throw c_NetError("Unable to receive listen data -- Socket Invalid!", ERR_LOW);
			} else if (errno == ECONNABORTED) {
				throw c_NetError("Unable to receive listen data -- Connection Aborted!", ERR_LOW);
			} else if (errno == ECONNREFUSED) {
				throw c_NetError("Unable to receive listen data -- Connection Refused!", ERR_LOW);
			} else if (errno == ECONNRESET) {
				throw c_NetError("Unable to receive listen data -- Connection Reset!", ERR_LOW);
			} else if (errno == EINTR) {
				throw c_NetError("Unable to receive listen data -- Operation Interrupted!", ERR_LOW);
			} else if (errno == EOPNOTSUPP) {
				throw c_NetError("Unable to receive listen data -- Socket Type Refused!", ERR_LOW);
			} else if (errno == ETIMEDOUT) {
				throw c_NetError("Unable to receive listen data -- Connection Timed Out!", ERR_LOW);
			} else if (errno == EFAULT) {
				throw c_NetError("Unable to receive listen data -- Invalid Buffer!", ERR_LOW);
			} else if (errno == ENOTCONN) {
				throw c_NetError("Unable to receive listen data -- Not Connected!", ERR_LOW);
			} else {
				throw c_NetError("Unable to receive listen data -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
			}
		}
	} else {
		return 0;
	}
	*ip_storePortFrom = ntohs(bruh.sin_port);
	u32 B = ntohl(bruh.sin_family);
	u8 nA = u8(B >> 24), nB = u8(B >> 16), nC = u8(B >> 8), nD = u8(B);
	c_IPv4Addr newer(nA, nB, nC, nD);
	*ip_storeDataFrom = newer;
	return 1;
}
//Returns # Of bytes sent
u32 c_IOSocket::transmitTCP(char* i_bufferItem, u32 i_bufferSize, bool i_last) {
	if (!this->m_socketized || !this->m_TCP) throw c_NetError("Unable to transmit data (TCP) -- Not Connected!", ERR_SEV);
	u32 result = send(this->m_operateSocket, i_bufferItem, i_bufferSize, (i_last) ? MSG_EOR : 0);
	if (result == i_bufferSize) return i_bufferSize;
	if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
		return 0;
	} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
		throw c_NetError("Unable to transmit data (TCP) -- Socket Invalid!", ERR_MED);
	} else if (errno == ECONNRESET) {
		throw c_NetError("Unable to transmit data (TCP) -- Connection Reset!", ERR_LOW);
	} else if (errno == ECONNREFUSED) {
		throw c_NetError("Unable to transmit data (TCP) -- Connection Refused!", ERR_LOW);
	} else if ((errno == EDESTADDRREQ) || (errno == ENOTCONN)) {
		throw c_NetError("Unable to transmit data (TCP) -- Socket Not Connected!", ERR_MED);
	} else if (errno == EFAULT) {
		throw c_NetError("Unable to transmit data (TCP) -- Invalid Buffer!", ERR_MED);
	} else if (errno == EINTR) {
		throw c_NetError("Unable to transmit data (TCP) -- Operation Interrupted!", ERR_LOW);
	} else if (errno == EMSGSIZE) {
		return result;
		//throw c_NetError("Unable to transmit data (TCP) -- Too Large!", ERR_MIN);
	} else if (errno == EOPNOTSUPP) {
		throw c_NetError("Unable to transmit data (TCP) -- Socket Type Refused Flags!", ERR_LOW);
	} else {
		throw c_NetError("Unable to transmit data (TCP) -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
	}
	return 1;
}

u32 c_IOSocket::transmitUDP(char* i_bufferItem, u32 i_bufferSize, sockaddr* i_recipient, bool i_last) {
	if (this->m_socketized || this->m_TCP) throw c_NetError("Unable to transmit data (UDP) -- Connected as TCP!", ERR_SEV);
	u32 result = sendto(this->m_Socket.m_socketDescriptor, i_bufferItem, i_bufferSize, (i_last) ? MSG_EOR : 0, i_recipient, sizeof(sockaddr));
	if (result == i_bufferSize) return i_bufferSize;
	if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
		return 0;
	} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
		throw c_NetError("Unable to transmit data (UDP) -- Socket Invalid!", ERR_MED);
	} else if (errno == ECONNRESET) {
		throw c_NetError("Unable to transmit data (UDP) -- Connection Reset!", ERR_LOW);
	} else if (errno == ECONNREFUSED) {
		throw c_NetError("Unable to transmit data (UDP) -- Connection Refused!", ERR_LOW);
	} else if ((errno == EDESTADDRREQ) || (errno == ENOTCONN)) {
		throw c_NetError("Unable to transmit data (UDP) -- Socket Not Connected!", ERR_MED);
	} else if (errno == EFAULT) {
		throw c_NetError("Unable to transmit data (UDP) -- Invalid Buffer!", ERR_MED);
	} else if (errno == EINTR) {
		throw c_NetError("Unable to transmit data (UDP) -- Operation Interrupted!", ERR_LOW);
	} else if (errno == EMSGSIZE) {
		return result;
		//throw c_NetError("Unable to transmit data (UDP) -- Too Large!", ERR_MIN);
	} else if (errno == EOPNOTSUPP) {
		throw c_NetError("Unable to transmit data (UDP) -- Socket Type Refused Flags!", ERR_LOW);
	} else {
		throw c_NetError("Unable to transmit data (UDP) -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
	}
	return 1;
}
u32 c_IOSocket::transmitUDP(char* i_bufferItem, u32 i_bufferSize, c_IPv4Addr i_recipient, u16 i_portRec, bool i_last) {
	if (this->m_socketized || this->m_TCP) throw c_NetError("Unable to transmit data (UDP) -- Connected as TCP!", ERR_SEV);
	sockaddr_in fullRec = i_recipient.makeCSocket(i_portRec);
	u32 result = sendto(this->m_Socket.m_socketDescriptor, i_bufferItem, i_bufferSize, (i_last) ? MSG_EOR : 0, (sockaddr*)&fullRec, sizeof(sockaddr));
	if (result == i_bufferSize) return i_bufferSize;
	if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
		return 0;
	} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
		throw c_NetError("Unable to transmit data (UDP) -- Socket Invalid!", ERR_MED);
	} else if (errno == ECONNRESET) {
		throw c_NetError("Unable to transmit data (UDP) -- Connection Reset!", ERR_LOW);
	} else if (errno == ECONNREFUSED) {
		throw c_NetError("Unable to transmit data (UDP) -- Connection Refused!", ERR_LOW);
	} else if ((errno == EDESTADDRREQ) || (errno == ENOTCONN)) {
		throw c_NetError("Unable to transmit data (UDP) -- Socket Not Connected!", ERR_MED);
	} else if (errno == EFAULT) {
		throw c_NetError("Unable to transmit data (UDP) -- Invalid Buffer!", ERR_MED);
	} else if (errno == EINTR) {
		throw c_NetError("Unable to transmit data (UDP) -- Operation Interrupted!", ERR_LOW);
	} else if (errno == EMSGSIZE) {
		return result;
		//throw c_NetError("Unable to transmit data (UDP) -- Too Large!", ERR_MIN);
	} else if (errno == EOPNOTSUPP) {
		throw c_NetError("Unable to transmit data (UDP) -- Socket Type Refused Flags!", ERR_LOW);
	} else {
		throw c_NetError("Unable to transmit data (UDP) -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
	}
	return 1;
}

} }
