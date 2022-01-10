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
// END OLD

void c_BaseSocket::deleteDescriptor() {
	if (this->mv_bound) {
		shutdown(this->mv_baseSocketDesc, SHUT_RDWR);
		this->mv_baseSocketDesc = 0;
	}
}

void c_BaseSocket::restoreDescriptor() {
	fcntl(this->mv_baseSocketDesc, F_SETFL, this->mv_oldSocketFlags);
}

void c_BaseSocket::bindAndConfigure() {
	if (this->mv_bound) return;
	if (!this->mv_initialized) throw c_NetError("Unable to configure unbound socket -- Socket Uninitialized!", ERR_FTL);
	
	
	this->mv_cSocket = this->mv_address.makeCSocket(this->mv_socketPort);
	int X = bind(this->mv_baseSocketDesc, (sockaddr*)&this->mv_cSocket, sizeof(sockaddr_in));
	
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
	
	int tmpOpt = 1;
	X = setsockopt(this->mv_baseSocketDesc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_KEEPALIVE, &tmpOpt, sizeof(tmpOpt));
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
bool c_BaseSocket::receive(char* i_bufferTo, u32 i_bufferSize) {
	if (!this->mv_bound || !this->mv_initialized) return 0;
	int readData = recv(this->mv_operateSocket, i_bufferTo, i_bufferSize, MSG_WAITALL);
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
	
	int X = socket(AF_INET, SOCK_DGRAM, 0);
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
		mv_oldSocketFlags = fcntl(mv_baseSocketDesc, F_GETFL, 0);
		fcntl(mv_baseSocketDesc, F_SETFL, mv_oldSocketFlags | O_NONBLOCK);
	}
	mv_initialized = 1;
	bindAndConfigure();
	
	mv_operateSocket = mv_baseSocketDesc;
}
c_Socket_UDP::c_Socket_UDP(c_IPv4Addr i_overAddr, u16 i_port, ConnWait i_waitForConn) {
	mv_socketPort = i_port;
	mv_address = i_overAddr;
	
	int X = socket(AF_INET, SOCK_DGRAM, 0);
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
		mv_oldSocketFlags = fcntl(mv_baseSocketDesc, F_GETFL, 0);
		fcntl(mv_baseSocketDesc, F_SETFL, mv_oldSocketFlags | O_NONBLOCK);
	}
	mv_initialized = 1;
	bindAndConfigure();
	
	mv_operateSocket = mv_baseSocketDesc;
}
c_Socket_UDP::~c_Socket_UDP() {
	this->deleteDescriptor();
}
bool c_Socket_UDP::receiveWithAddr(char* i_bufferTo, u32 i_bufferSize, c_IPv4Addr* ip_addressTo, u16* ip_portTo) {
	sockaddr_in tempbruh; unsigned int bruhSize = sizeof(tempbruh);
	if (!this->mv_bound || !this->mv_initialized) return 0;
	
	int readStatus = recvfrom(this->mv_operateSocket, i_bufferTo, i_bufferSize, MSG_WAITALL, (sockaddr*)&tempbruh, &bruhSize);
	
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
	
	*ip_portTo = ntohs(tempbruh.sin_port);
	u32 B = ntohl(tempbruh.sin_family);
	u8 nA = u8(B >> 24), nB = u8(B >> 16), nC = u8(B >> 8), nD = u8(B);
	c_IPv4Addr newer(nA, nB, nC, nD);
	*ip_addressTo = newer;
	return 1;
}
u32 c_Socket_UDP::transmit(char* i_bufferFrom, u32 i_bufferSize, bool i_endOfRecord, c_IPv4Addr i_addressTo, u16 i_portTo) {
	if (!this->mv_bound || !this->mv_initialized) throw c_NetError("Unable to transmit UDP data -- System not ready!", ERR_MED);
	
	sockaddr_in fullRec = i_addressTo.makeCSocket(i_portTo);
	
	u32 result = sendto(this->mv_operateSocket, i_bufferFrom, i_bufferSize, (i_endOfRecord) ? MSG_EOR : 0, (sockaddr*)&fullRec, sizeof(fullRec));
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
	
	int X = socket(AF_INET, SOCK_STREAM, 0);
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
		mv_oldSocketFlags = fcntl(mv_baseSocketDesc, F_GETFL, 0);
		fcntl(mv_baseSocketDesc, F_SETFL, mv_oldSocketFlags | O_NONBLOCK);
	}
	mv_initialized = 1;
	
	bindAndConfigure();
	
	m_baseMode = i_setupMode;
	
	if (i_setupMode == TCP_SETUP_LISTEN) {
		int X = listen(mv_baseSocketDesc, 3);
		
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
	
	int X = socket(AF_INET, SOCK_STREAM, 0);
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
		mv_oldSocketFlags = fcntl(mv_baseSocketDesc, F_GETFL, 0);
		fcntl(mv_baseSocketDesc, F_SETFL, mv_oldSocketFlags | O_NONBLOCK);
	}
	mv_initialized = 1;
	
	bindAndConfigure();
	
	m_baseMode = i_setupMode;
	if (i_setupMode == TCP_SETUP_LISTEN) {
		int X = listen(mv_baseSocketDesc, 3);
		
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
	int X = accept(this->mv_baseSocketDesc, (sockaddr*)&this->mv_cSocket, (socklen_t*)&leneg);
	
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
	
	sockaddr_in tmpgoo = i_client.makeCSocket(i_port);
	int X = connect(this->mv_baseSocketDesc, (sockaddr*)&tmpgoo, sizeof(tmpgoo));
	
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
	
	sockaddr_in bruh; unsigned int bruhSize = sizeof(bruh);
	
	int gval = getpeername(this->mv_operateSocket, (sockaddr*)&bruh, &bruhSize);
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
	
	*ip_port = ntohs(bruh.sin_port);
	u32 B = ntohl(bruh.sin_family);
	u8 nA = u8(B >> 24), nB = u8(B >> 16), nC = u8(B >> 8), nD = u8(B);
	c_IPv4Addr newer(nA, nB, nC, nD);
	*ip_addr = newer;
	return 1;
}
u32 c_Socket_TCP::transmit(char* i_bufferFrom, u32 i_bufferSize, bool i_endOfRecord) {
	if (!this->m_connected) throw c_NetError("Unable to transmit TCP data -- Not connected!", ERR_MIN);
	
	u32 result = send(this->mv_operateSocket, i_bufferFrom, i_bufferSize, (i_endOfRecord) ? MSG_EOR : 0);
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

} }
