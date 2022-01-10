#include "Networker Refined.hpp"

namespace ERCLIB { namespace Net {
//Start OLD
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

const char* c_NetError::what() const noexcept {
	return this->m_about.c_str();
}

const ErrorLevel c_NetError::getLevel() const noexcept {
	return this->m_severity;
}

const std::string c_NetError::fullWhat() const noexcept {
	std::string N = '[' + errLvlTxt(this->m_severity) + "] " + this->m_about;
	return N;
}


c_IPv4Addr::c_IPv4Addr() {
	A = 127; //Loopback
	B = 0;
	C = 0;
	D = 1;
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

const Clib::sockaddr_in c_IPv4Addr::makeCSocket(u16 i_port) const noexcept {
	Clib::sockaddr_in N;
	N.sin_family = AF_INET;
	N.sin_addr.s_addr = Clib::htonl(this->toUint());
	N.sin_port = Clib::htons(i_port);
	return N;
}
// END OLD

void c_BaseSocket::deleteDescriptor() {
	if (this->mv_bound) {
		Clib::shutdown(this->mv_baseSocketDesc, Clib::SHUT_RDWR);
	}
	Clib::close(this->mv_baseSocketDesc);
	this->mv_baseSocketDesc = 0;
}

void c_BaseSocket::restoreDescriptor() {
	Clib::fcntl(this->mv_baseSocketDesc, F_SETFL, this->mv_oldSocketFlags);
}

void c_BaseSocket::bindAndConfigure() {
	if (this->mv_bound) return;
	if (!this->mv_initialized) throw c_NetError("Unable to configure unbound socket -- Socket Uninitialized!", ERR_FTL);
	
	
	int tmpOpt = 1;
	
	this->mv_cSocket = this->mv_address.makeCSocket(this->mv_socketPort);
	int X = Clib::setsockopt(this->mv_baseSocketDesc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_KEEPALIVE, &tmpOpt, sizeof(tmpOpt));
	if (X != 0){
		if (errno == EBADF) {
			throw c_NetError("Unable to configure bound socket -- Unknown Socket Error!", ERR_FTL);
		} else if (errno == EDOM) {
			throw c_NetError("Unable to configure bound socket -- Timeout Domain Error!", ERR_FTL);
		} else if (errno == EFAULT) {
			throw c_NetError("Unable to configure bound socket -- Invalid Option Pointer!", ERR_FTL);
		} else if (errno == EINVAL) {
			throw c_NetError("Unable to configure bound socket -- Invalid Options!", ERR_FTL);
		} else if (errno == EISCONN) {
			throw c_NetError("Unable to configure bound socket -- Socket In Use!", ERR_FTL);
		} else if (errno == ENOPROTOOPT) {
			throw c_NetError("Unable to configure bound socket -- Protocol Options Not Supported!", ERR_FTL);
		} else if (errno == ENOTSOCK) {
			throw c_NetError("Unable to configure bound socket -- Descriptor is not a Socket!", ERR_FTL);
		} else {
			throw c_NetError("Unable to configure bound socket -- Unknown Failure (" + std::to_string(errno) + ')', ERR_FTL);
		}
	}
	
	X = Clib::bind(this->mv_baseSocketDesc, (Clib::sockaddr*)&this->mv_cSocket, sizeof(Clib::sockaddr_in));
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
	this->mv_bound = 1;
}

const bool c_BaseSocket::getStateBound() const noexcept {
	return this->mv_bound;
}
const bool c_BaseSocket::getStateInitialized() const noexcept {
	return this->mv_initialized;
}
const ConnWait c_BaseSocket::getStateWaitConnect() const noexcept {
	return this->mv_waitConnect;
}
const u16 c_BaseSocket::getPortInUse() const noexcept {
	return this->mv_socketPort;
}
const c_IPv4Addr c_BaseSocket::getAddressInUse() const noexcept {
	return this->mv_address;
}
const int& c_BaseSocket::r_getSocketDescriptor() const noexcept {
	return this->mv_baseSocketDesc;
}
const int& c_BaseSocket::r_getOperatingSocket() const noexcept {
	return this->mv_operateSocket;
}
bool c_BaseSocket::receive(char* i_bufferTo, u16 i_bufferSize) {
	if (!this->mv_bound || !this->mv_initialized) return 0;
	int readData = Clib::recv(this->mv_operateSocket, i_bufferTo, i_bufferSize, 0);
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
	return 1;
}
void c_BaseSocket::shutdownSocket() {
	this->restoreDescriptor();
	this->deleteDescriptor();
}


c_Socket_UDP::c_Socket_UDP(u16 i_port, ConnWait i_waitForConn) {
	mv_socketPort = i_port;
	c_IPv4Addr autoBack;
	mv_address = autoBack;
	
	int X = Clib::socket(AF_INET, Clib::SOCK_DGRAM, 0);
	if (X <= 0) {
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
	mv_baseSocketDesc = X;
	mv_waitConnect = i_waitForConn;
	
	if (i_waitForConn == NO_WAIT) {
		mv_oldSocketFlags = Clib::fcntl(mv_baseSocketDesc, F_GETFL, 0);
		Clib::fcntl(mv_baseSocketDesc, F_SETFL, mv_oldSocketFlags | O_NONBLOCK);
	}
	mv_initialized = 1;
	bindAndConfigure();
	
	mv_operateSocket = mv_baseSocketDesc;
}
c_Socket_UDP::c_Socket_UDP(c_IPv4Addr i_overAddr, u16 i_port, ConnWait i_waitForConn) {
	mv_socketPort = i_port;
	mv_address = i_overAddr;
	
	int X = Clib::socket(AF_INET, Clib::SOCK_DGRAM, 0);
	if (X <= 0) {
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
	mv_baseSocketDesc = X;
	mv_waitConnect = i_waitForConn;
	
	if (i_waitForConn == NO_WAIT) {
		mv_oldSocketFlags = Clib::fcntl(mv_baseSocketDesc, F_GETFL, 0);
		Clib::fcntl(mv_baseSocketDesc, F_SETFL, mv_oldSocketFlags | O_NONBLOCK);
	}
	mv_initialized = 1;
	bindAndConfigure();
	
	mv_operateSocket = mv_baseSocketDesc;
}
c_Socket_UDP::~c_Socket_UDP() {
	this->deleteDescriptor();
}
bool c_Socket_UDP::receiveWithAddr(char* i_bufferTo, u16 i_bufferSize, c_IPv4Addr* ip_addressTo, u16* ip_portTo) {
	Clib::sockaddr_in tempbruh; unsigned int bruhSize = sizeof(tempbruh);
	if (!this->mv_bound || !this->mv_initialized) return 0;
	
	int readStatus = Clib::recvfrom(this->mv_operateSocket, i_bufferTo, i_bufferSize, 0, (Clib::sockaddr*)&tempbruh, &bruhSize);
	
	if (readStatus == 0) return 0;
	if (readStatus < 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
			return false; //We aren't waiting for a packet
		} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
			throw c_NetError("Unable to receive UDP listen data -- Socket Invalid!", ERR_LOW);
		} else if (errno == ECONNABORTED) {
			throw c_NetError("Unable to receive UDP listen data -- Connection Aborted!", ERR_LOW);
		} else if (errno == ECONNREFUSED) {
			throw c_NetError("Unable to receive UDP listen data -- Connection Refused!", ERR_LOW);
		} else if (errno == ECONNRESET) {
			throw c_NetError("Unable to receive UDP listen data -- Connection Reset!", ERR_LOW);
		} else if (errno == EINTR) {
			throw c_NetError("Unable to receive UDP listen data -- Operation Interrupted!", ERR_LOW);
		} else if (errno == EOPNOTSUPP) {
			throw c_NetError("Unable to receive UDP listen data -- Socket Type Refused!", ERR_LOW);
		} else if (errno == ETIMEDOUT) {
			throw c_NetError("Unable to receive UDP listen data -- Connection Timed Out!", ERR_LOW);
		} else if (errno == EFAULT) {
			throw c_NetError("Unable to receive UDP listen data -- Invalid Buffer!", ERR_LOW);
		} else if (errno == ENOTCONN) {
			throw c_NetError("Unable to receive UDP listen data -- Not Connected!", ERR_LOW);
		} else {
			throw c_NetError("Unable to receive UDP listen data -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
		}
	}
	
	*ip_portTo = Clib::ntohs(tempbruh.sin_port);
	u32 B = Clib::ntohl(tempbruh.sin_addr.s_addr);
	u8 nA = u8(B >> 24), nB = u8(B >> 16), nC = u8(B >> 8), nD = u8(B);
	c_IPv4Addr newer(nA, nB, nC, nD);
	*ip_addressTo = newer;
	return 1;
}
u32 c_Socket_UDP::transmit(char* i_bufferFrom, u16 i_bufferSize, bool i_endOfRecord, c_IPv4Addr i_addressTo, u16 i_portTo) {
	if (!this->mv_bound || !this->mv_initialized) throw c_NetError("Unable to transmit UDP data -- System not ready!", ERR_MED);
	
	Clib::sockaddr_in fullRec = i_addressTo.makeCSocket(i_portTo);
	
	u32 result = Clib::sendto(this->mv_operateSocket, i_bufferFrom, i_bufferSize, (i_endOfRecord) ? Clib::MSG_EOR : 0, (Clib::sockaddr*)&fullRec, sizeof(fullRec));
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
	return 0; //Error Case
}



c_Socket_TCP::c_Socket_TCP(u16 i_port, ConnWait i_waitForConn, TCPSetup i_setupMode) {
	mv_socketPort = i_port;
	c_IPv4Addr autoBack;
	mv_address = autoBack;
	
	int X = Clib::socket(AF_INET, Clib::SOCK_STREAM, 0);
	if (X <= 0) {
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
	mv_baseSocketDesc = X;
	mv_waitConnect = i_waitForConn;
	
	if (i_waitForConn == NO_WAIT) {
		mv_oldSocketFlags = Clib::fcntl(mv_baseSocketDesc, F_GETFL, 0);
		Clib::fcntl(mv_baseSocketDesc, F_SETFL, mv_oldSocketFlags | O_NONBLOCK);
	}
	mv_initialized = 1;
	
	bindAndConfigure();
	
	m_baseMode = i_setupMode;
	
	if (i_setupMode == TCP_SETUP_LISTEN) {
		int X = Clib::listen(mv_baseSocketDesc, 3);
		
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
	}
}
c_Socket_TCP::c_Socket_TCP(c_IPv4Addr i_overAddr, u16 i_port, ConnWait i_waitForConn, TCPSetup i_setupMode) {
	mv_socketPort = i_port;
	mv_address = i_overAddr;
	
	int X = Clib::socket(AF_INET, Clib::SOCK_STREAM, 0);
	if (X <= 0) {
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
	mv_baseSocketDesc = X;
	mv_waitConnect = i_waitForConn;
	
	if (i_waitForConn == NO_WAIT) {
		mv_oldSocketFlags = Clib::fcntl(mv_baseSocketDesc, F_GETFL, 0);
		Clib::fcntl(mv_baseSocketDesc, F_SETFL, mv_oldSocketFlags | O_NONBLOCK);
	}
	mv_initialized = 1;
	
	bindAndConfigure();
	
	m_baseMode = i_setupMode;
	if (i_setupMode == TCP_SETUP_LISTEN) {
		int X = Clib::listen(mv_baseSocketDesc, 3);
		
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
	}
}
c_Socket_TCP::~c_Socket_TCP() {
	this->deleteDescriptor();
}
bool c_Socket_TCP::connectionAccept() {
	if (this->m_baseMode != TCP_SETUP_LISTEN) throw c_NetError("Unable to accept connection -- IOSocket not in TCP Listening mode!", ERR_MIN);
	
	unsigned int leneg = sizeof(this->mv_cSocket);
	int X = Clib::accept(this->mv_baseSocketDesc, (Clib::sockaddr*)&this->mv_cSocket, (Clib::socklen_t*)&leneg);
	
	if (X <= 0) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
			return false; //Nobody wants to connect :(
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
	this->mv_operateSocket = X;
	this->m_connected = 1;
	return 1;
}
bool c_Socket_TCP::connectionEstablish(c_IPv4Addr i_client, u16 i_port) {
	if (this->m_baseMode != TCP_SETUP_CONNECT) throw c_NetError("Unable to establish connection -- IOSocket not in TCP Connect mode!", ERR_MIN);
	
	Clib::sockaddr_in tmpgoo = i_client.makeCSocket(i_port);
	int X = Clib::connect(this->mv_baseSocketDesc, (Clib::sockaddr*)&tmpgoo, sizeof(tmpgoo));
	
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
	
	this->mv_operateSocket = this->mv_baseSocketDesc;
	this->m_connected = 1;
	return 1;
}
bool c_Socket_TCP::getClientInfo(c_IPv4Addr* ip_addr, u16* ip_port) {
	if (!this->m_connected) return 0;
	
	Clib::sockaddr_in bruh; unsigned int bruhSize = sizeof(bruh);
	
	int gval = Clib::getpeername(this->mv_operateSocket, (Clib::sockaddr*)&bruh, &bruhSize);
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
	
	*ip_port = Clib::ntohs(bruh.sin_port);
	u32 B = Clib::ntohl(bruh.sin_addr.s_addr);
	u8 nA = u8(B >> 24), nB = u8(B >> 16), nC = u8(B >> 8), nD = u8(B);
	c_IPv4Addr newer(nA, nB, nC, nD);
	*ip_addr = newer;
	return 1;
}
u32 c_Socket_TCP::transmit(char* i_bufferFrom, u16 i_bufferSize, bool i_endOfRecord) {
	if (!this->m_connected) throw c_NetError("Unable to transmit TCP data -- Not connected!", ERR_MIN);
	
	u32 result = Clib::send(this->mv_operateSocket, i_bufferFrom, i_bufferSize, (i_endOfRecord) ? Clib::MSG_EOR : 0);
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
	return 0; //error case
}


c_TCP_Subsock::c_TCP_Subsock(int& i_descriptor) : m_mainDesc(i_descriptor) {};



c_TCP_Server::c_TCP_Server(u16 i_port, short i_clients) : m_maxClients(i_clients) {
	m_sharedPort = i_port;
	c_IPv4Addr autoBack;
	m_baseAddress = autoBack;
	m_cSocket = m_baseAddress.makeCSocket(i_port);
	
	if (i_clients <= 0) throw c_NetError("Cannot establish a server with " + std::to_string(i_clients) + " clients!", ERR_FTL);
	
	int X = Clib::socket(AF_INET, Clib::SOCK_STREAM, 0);
	if (X <= 0) {
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
	m_masterSocket = X;
	
	int tmpOpt = 1;
	X = Clib::setsockopt(m_masterSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_KEEPALIVE, &tmpOpt, sizeof(tmpOpt));
	if (X != 0) {
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
		
	int oFlags = Clib::fcntl(m_masterSocket, F_GETFL, 0);
	Clib::fcntl(m_masterSocket, F_SETFL, oFlags | O_NONBLOCK);

	X = Clib::bind(m_masterSocket, (Clib::sockaddr*)&m_cSocket, sizeof(m_cSocket));
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
	FD_ZERO(&m_fdHandler);
	FD_SET(m_masterSocket, &m_fdHandler);
	m_currentMaxFD = m_masterSocket;
	
	for (short i=0; i<m_maxClients; i++) {
		c_TCP_Subsock xFile(m_masterSocket);
		m_cliSocks.push_back(xFile);
	}
	X = Clib::listen(m_masterSocket, 6); // up to 6 pending connections
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
	m_masterPrep = 1;
}
c_TCP_Server::c_TCP_Server(c_IPv4Addr i_overAddr, u16 i_port, short i_clients) : m_maxClients(i_clients) {
	m_sharedPort = i_port;
	m_baseAddress = i_overAddr;
	m_cSocket = m_baseAddress.makeCSocket(i_port);
	if (i_clients <= 0) throw c_NetError("Cannot establish a server with " + std::to_string(i_clients) + " clients!", ERR_FTL);
	
	
	int X = Clib::socket(AF_INET, Clib::SOCK_STREAM, 0);
	if (X <= 0) {
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
	m_masterSocket = X;
	
	int tmpOpt = 1;
	X = Clib::setsockopt(m_masterSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_KEEPALIVE, &tmpOpt, sizeof(tmpOpt));
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
	X = Clib::bind(m_masterSocket, (Clib::sockaddr*)&m_cSocket, sizeof(m_cSocket));
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
	FD_ZERO(&m_fdHandler);
	FD_SET(m_masterSocket, &m_fdHandler);
	m_currentMaxFD = m_masterSocket;
	
	for (u16 i=0; i<u16(m_maxClients); i++) {
		c_TCP_Subsock xFile(m_masterSocket);
		m_cliSocks.push_back(xFile);
	}
	X = Clib::listen(m_masterSocket, 6); // up to 6 other connections
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
	
	m_masterPrep = 1;
}

const short c_TCP_Server::getActiveClients() const noexcept {
	return this->m_currentClients;
}
const u16 c_TCP_Server::getPortInUse() const noexcept {
	return this->m_sharedPort;
}
const c_IPv4Addr c_TCP_Server::getAddressInUse() const noexcept {
	return this->m_baseAddress;
}

const std::vector<short> c_TCP_Server::singleLoopOp() {
	FD_ZERO(&this->m_fdHandler);
	FD_SET(this->m_masterSocket, &this->m_fdHandler);
	this->m_currentMaxFD = this->m_masterSocket;
	
	for (u16 i=0; i<u16(this->m_maxClients); i++) {
		auto sd = this->m_cliSocks.at(i);
		
		if (sd.m_connected) {
			FD_SET(sd.m_opDesc, &this->m_fdHandler);
			if (sd.m_opDesc > this->m_currentMaxFD) {
				this->m_currentMaxFD = sd.m_opDesc;
			}
		}
	}
	
	std::vector<short> statusVec;
	
	timeval X; X.tv_sec = 0; X.tv_usec = 125000; //0.0125 second wait for something to happen.
	int activity = select(this->m_currentMaxFD + 1, &this->m_fdHandler, NULL, NULL, &X);
	if (activity < 0) {
		if (errno == EINTR) return statusVec;
		throw c_NetError("Error while trying to wait for activity on server (" + std::to_string(errno) + ')', ERR_SEV);
	} else if (activity == 0) {
		return statusVec; //Nothing happened :(
	} else {
		//Something happened!
		if (FD_ISSET(this->m_masterSocket, &this->m_fdHandler)) {
			//New inbound connection
			auto xsize = sizeof(this->m_cSocket);
			int X = 1;
			
			do {
				X = Clib::accept(this->m_masterSocket, (Clib::sockaddr*)&this->m_cSocket, (Clib::socklen_t*)&xsize);
				if (X <= 0) {
					if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
						break; //Nobody wants to connect :(
					} else if ((errno == EBADF) || (errno == ENOTSOCK)) {
						throw c_NetError("Unable to accept connection in server -- Socket Invalid!", ERR_LOW);
					} else if (errno == ECONNABORTED) {
						throw c_NetError("Unable to accept connection in server -- Connection Aborted!", ERR_LOW);
					} else if (errno == ECONNREFUSED) {
						throw c_NetError("Unable to accept connection in server -- Connection Refused!", ERR_LOW);
					} else if (errno == EINTR) {
						throw c_NetError("Unable to accept connection in server -- Operation Interrupted!", ERR_LOW);
					} else if ((errno == EMFILE) || (errno == ENFILE)) {
						throw c_NetError("Unable to accept connection -- Too Many Active Connections!", ERR_LOW);
					} else if (errno == EOPNOTSUPP) {
						throw c_NetError("Unable to accept connection in server -- Socket Type Refused!", ERR_LOW);
					} else {
						throw c_NetError("Unable to accept connection in server -- Unknown Failure (" + std::to_string(errno) + ')', ERR_LOW);
					}
				}
				//Assign it
				for (u16 i=0; i<u16(this->m_maxClients); i++) {
					if (this->m_cliSocks.at(i).m_connected == false) {
						this->m_cliSocks.at(i).m_opDesc = X;
						this->m_cliSocks.at(i).m_connected = 1;
						this->m_currentClients++;
						
						statusVec.push_back(i * -1);
						X = -5;
						break; //Escape current level to next X iteration
					} else if (i == u16(this->m_maxClients)) {
						throw c_NetError("Too many clients connected to server!", ERR_MED);
					}
					Clib::usleep(1); //Wait one microsecond
				}
			} while (X < 0);
			//Loops through as many connections as we have pending
		}
		//Test if any existing connection(s) active
		for (u16 i=0; i<u16(this->m_maxClients); i++) {
			int sd = this->m_cliSocks.at(i).m_opDesc;
			if (FD_ISSET(sd, &this->m_fdHandler) && this->m_cliSocks.at(i).m_connected) {
				statusVec.push_back(i);
			}
			Clib::usleep(1); //Wait one microsecond inbetween checks
		}
	}
	return statusVec;
}
const bool c_TCP_Server::subsockExists(short i_socket) const noexcept {
	return (i_socket < this->m_maxClients) && (i_socket >= 0);
}
const bool c_TCP_Server::isSubsockConnected(short i_socket) const noexcept {
	if (i_socket >= this->m_maxClients) return false;
	if (i_socket < 0) return false;
	return (this->m_cliSocks[i_socket].m_connected);
}
const c_IPv4Addr c_TCP_Server::getSubsockClient(short i_socket) const {
	if (!this->isSubsockConnected(i_socket)) throw c_NetError("Cannot get client information from unconnected/invalid subsocket!", ERR_MED);
	
	auto sub = &this->m_cliSocks[i_socket];
	
	Clib::sockaddr_in X; unsigned int xSize = sizeof(X);
	int gval = Clib::getpeername(sub->m_opDesc, (Clib::sockaddr*)&X, &xSize);
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

	u32 B = Clib::ntohl(X.sin_addr.s_addr);
	u8 nA = u8(B >> 24), nB = u8(B >> 16), nC = u8(B >> 8), nD = u8(B);
	c_IPv4Addr newer(nA, nB, nC, nD);
	return newer;
}

const bool c_TCP_Server::receiveOnSubsock(short i_socket, char* ip_buff, u16 i_buffSize) {
	if (!this->isSubsockConnected(i_socket)) return false;
	
	auto subsockPtr = &this->m_cliSocks[i_socket];
	
	int readData = Clib::recv(subsockPtr->m_opDesc, ip_buff, i_buffSize, 0);
	
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
	return 1;
}
const u32 c_TCP_Server::transmitOnSubsock(short i_socket, char* ip_buff, u16 i_buffSize, bool i_eor) {
	if (!this->isSubsockConnected(i_socket)) return 0;
	
	auto subsockPtr = &this->m_cliSocks[i_socket];

	int result = Clib::send(subsockPtr->m_opDesc, ip_buff, i_buffSize, (i_eor) ? Clib::MSG_EOR : 0);
	
	if (result == int(i_buffSize)) return i_buffSize;
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
	return 0;
}
const bool c_TCP_Server::shutdownSubsock(short i_socket) {
	if (!this->isSubsockConnected(i_socket)) return 0;
	auto X = &this->m_cliSocks[i_socket];
	Clib::shutdown(X->m_opDesc, Clib::SHUT_RDWR);
	Clib::close(X->m_opDesc);
	X->m_connected = 0; X->m_opDesc = 0;
	this->m_currentClients--;
	return 1;
}
const bool c_TCP_Server::shutdownServer() {
	if (!this->m_masterPrep) return 0;
	for (u16 i=0; i<u16(this->m_maxClients); i++) {
		this->shutdownSubsock(i);
	}
	Clib::shutdown(this->m_masterSocket, Clib::SHUT_RDWR);
	Clib::close(this->m_masterSocket);
	this->m_masterSocket = 0;
	this->m_masterPrep = 0;
	FD_ZERO(&this->m_fdHandler);
	return 1;
}

} }
