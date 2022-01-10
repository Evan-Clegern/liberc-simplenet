#ifndef ERCLIB_Networker
#define ERCLIB_Networker

#include <exception>
#include <iostream>
#include <ctime>
#include <string>
#include <vector>
#include <cstdint>
#include <cerrno>


typedef std::uint_fast32_t u32;
typedef std::uint_fast16_t u16;
typedef std::uint_fast64_t u64;
typedef std::uint_fast8_t  u8;


namespace ERCLIB { namespace Net {

// Networking Headers
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h> // for nonblocking mode
#include <unistd.h> // for closing descriptors, apparently
#include <sys/time.h>

/********!
 * @name ConnWait
 * 
 * @brief
 * 			When establishing an IOSocket, defines whether we want to wait when
 * 			attempting to create or accept a TCP connection, or receive a datagram.
 * 
 ********/
typedef enum : bool {
	NO_WAIT = 0,
	DO_WAIT = 1
} ConnWait;

/********!
 * @name TCPSetup
 * 
 * @brief
 * 			Specifies whether we are establishing a TCP IOSocket for actively listening
 * 			or for connecting to an external listening port.
 * 
 ********/
typedef enum : bool {
	TCP_SETUP_LISTEN = 0,
	TCP_SETUP_CONNECT = 1
} TCPSetup;

/********!
 * @name ErrorLevel
 * 
 * @brief
 * 			Specifies the severity of a THROW condition.
 ********/
typedef enum : u8 {
	ERR_MIN,
	ERR_LOW,
	ERR_MED,
	ERR_SEV, //Severe
	ERR_FTL  //Fatal
} ErrorLevel;

/********!
 * @name AddressClass
 * 
 * @brief
 * 			Specifies the general class of an IP address.
 * 
 ********/
typedef enum : u8 {
	CLASS_A,
	CLASS_B,
	CLASS_C,
	MULTICAST,
	EXPERIMENT,
	LOOPBACK
} AddressClass;

//! Turns an ErrorLevel item into a printable string.
std::string errLvlTxt(ErrorLevel level);

//! Exception class for the networking library.
class c_NetError : public std::exception {
	std::string m_about;
	ErrorLevel m_severity;
public:
	explicit c_NetError(const char* text, const ErrorLevel level);
	explicit c_NetError(const std::string text, const ErrorLevel level);
	const char* what() const noexcept;
	const ErrorLevel getLevel() const noexcept;
	const std::string fullWhat() const noexcept;
};


//! Wrapper for IP version 4 Addresses.
struct c_IPv4Addr {
	u8 A, B, C, D;
	explicit c_IPv4Addr(u8 a, u8 b, u8 c, u8 d) : A(a), B(b), C(c), D(d) {};
	explicit c_IPv4Addr();
	
	//! Assignment Operator.
	void operator=(const c_IPv4Addr& b);
	
	const AddressClass getClass() const noexcept;
	const bool isPrivate() const noexcept;
	const bool isAPIPA() const noexcept;
	const bool isLocalLoopback() const noexcept;
	const u32 toUint() const noexcept;
	const std::string toText() const noexcept;
	//! Creates a "C Socket" for use in the "sys/sockets.h" interface.
	const sockaddr_in makeCSocket(u16 i_port) const noexcept;
};

//! Virtual Class which handles most of the initialization and storage of a system-level socket and address.
class c_BaseSocket {
protected:
	bool mv_bound = 0; //! Whether we're bound to our specific port or not; sanity check.
	bool mv_initialized = 0; //! Whether or not we've been fully set up or not; sanity check.
	ConnWait mv_waitConnect = DO_WAIT; //! Whether or not we're going to wait for a connection.
	int mv_baseSocketDesc; //! Automatically-assigned file descriptor integer for our socket.
	c_IPv4Addr mv_address; //! Our IPv4 address we've given ourselves; hopefully just the local one.
	u16 mv_socketPort; //! The port we've bound ourselves to.
	sockaddr_in mv_cSocket; //! The "C Socket" representation of the port and IPv4Addr.
	int mv_oldSocketFlags; //! For shutdown/release purposes.
	int mv_operateSocket; //! Operating socket device
	
	void deleteDescriptor();
	void restoreDescriptor();
	void bindAndConfigure();
public:
	virtual const bool getStateBound() const noexcept;
	virtual const bool getStateInitialized() const noexcept;
	virtual const ConnWait getStateWaitConnect() const noexcept;
	virtual const u16  getPortInUse() const noexcept;
	virtual const c_IPv4Addr getAddressInUse() const noexcept;
	virtual const int& r_getSocketDescriptor() const noexcept; //Immutable reference
	virtual const int& r_getOperatingSocket() const noexcept;
	
	virtual bool receive(char* i_bufferTo, u16 i_bufferSize);
	virtual void shutdownSocket();
};

//! A UDP handlng socket.
class c_Socket_UDP : public c_BaseSocket {
public:
	explicit c_Socket_UDP(u16 i_port, ConnWait i_waitForConn);
	explicit c_Socket_UDP(c_IPv4Addr i_overAddr, u16 i_port, ConnWait i_waitForConn);
	~c_Socket_UDP();
	
	bool receiveWithAddr(char* i_bufferTo, u16 i_bufferSize, c_IPv4Addr* ip_addressTo, u16* ip_portTo);
	u32 transmit(char* i_bufferFrom, u16 i_bufferSize, bool i_endOfRecord, c_IPv4Addr i_addressTo, u16 i_portTo);
};

//! A TCP handling socket, with a fixed input or output mode. 
class c_Socket_TCP : public c_BaseSocket {
	TCPSetup m_baseMode;
	bool m_connected = 0;
public:
	explicit c_Socket_TCP(u16 i_port, ConnWait i_waitForConn, TCPSetup i_setupMode);
	explicit c_Socket_TCP(c_IPv4Addr i_overAddr, u16 i_port, ConnWait i_waitForConn, TCPSetup i_setupMode);
	~c_Socket_TCP();
	
	bool connectionAccept();
	bool connectionEstablish(c_IPv4Addr i_client, u16 i_port);
	
	bool getClientInfo(c_IPv4Addr* ip_addr, u16* ip_port);
	
	u32 transmit(char* i_bufferFrom, u16 i_bufferSize, bool i_endOfRecord);
};

//! Contains basic information for a Subsocket.
struct c_TCP_Subsock {
	int& m_mainDesc, m_opDesc=0;
	bool m_connected=false;
	
	explicit c_TCP_Subsock(int& i_descriptor);
};

//! Abstracted from c_BaseSocket. Contains multiple TCP Listening sockets.
//! None of the sockets are in DO_WAIT (i.e. all in NONBLOCK mode)
class c_TCP_Server {
	c_IPv4Addr m_baseAddress;
	u16 m_sharedPort;
	int m_masterSocket;
	short m_currentClients = 0;
	bool m_masterPrep = 0;
	const short m_maxClients;
	
	sockaddr_in m_cSocket;
	std::vector<c_TCP_Subsock> m_cliSocks;
	
	int m_currentMaxFD;
	fd_set m_fdHandler;
public:
	explicit c_TCP_Server(u16 i_port, short i_clients=0x7FFF);
	explicit c_TCP_Server(c_IPv4Addr i_overAddr, u16 i_port, short i_clients=0x7FFF);
	
	const short getActiveClients() const noexcept;
	const u16 getPortInUse() const noexcept;
	const c_IPv4Addr getAddressInUse() const noexcept;
	
	//! Function intended to be run FIRST in a loop.
	//! Returns a zero-length vector if nothing happened,
	//! Otherwise, returns a vector of sockets. Positive --> Waiting; Negative --> New.
	//! NOTE: The first client (0) can't be negative, understandably, so check as (X <= 0) not (X < 0)
	const std::vector<short> singleLoopOp();
	
	const bool subsockExists(short i_socket) const noexcept;
	const bool isSubsockConnected(short i_socket) const noexcept;
	const c_IPv4Addr getSubsockClient(short i_socket) const;
	
	const bool receiveOnSubsock(short i_socket, char* ip_buff, u16 i_buffSize);
	const u32  transmitOnSubsock(short i_socket, char* ip_buff, u16 i_buffSize, bool i_eor);
	const bool shutdownSubsock(short i_socket);
	
	const bool shutdownServer();
};

}
}

#endif
