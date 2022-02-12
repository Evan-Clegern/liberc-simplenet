#ifndef ERCLIB_Networker
#define ERCLIB_Networker

//Comment this line out if you want to gamble with std::uint_fast*_t types
#define EXPLICIT_UINT_SIZES

//Comment this line out if you want to enable a lot of debug messages
#define DISABLE_DEBUG_MESSAGES


#if __cplusplus < 201703L
	#error
#endif

#include <exception>
#include <iostream>
#include <ctime>
#include <string>
#include <vector>
#include <cerrno>


#ifdef EXPLICIT_UINT_SIZES
	typedef unsigned short u16;
	typedef unsigned char u8;
	typedef unsigned long int u32;
	typedef unsigned long long int u64;
#else
	#include <cstdint>
	typedef std::uint_fast16_t u16;
	typedef std::uint_fast8_t  u8;
	typedef std::uint_fast32_t u32;
	typedef std::uint_fast64_t u64;
#endif



namespace ERCLIB { namespace Net {
	
//! For custom use in UDP or TCP, just as a reference
namespace ControlChars {
	char CtrlNUL = 0   ; //! NULL byte
	char CtrlSOH = 1   ; //! Start of Heading
	char CtrlSTX = 2   ; //! Start of Text
	char CtrlETX = 3   ; //! End of Text
	char CtrlEOT = 4   ; //! End of Transmission
	char CtrlENQ = 5   ; //! Enquiry (for info about other device)
	char CtrlACK = 6   ; //! Acknowledgement (message received)
	char CtrlBEL = 7   ; //! Bell (ring ring)
	char CtrlBS  = 8   ; //! Backspace
	char CtrlHT  = 9   ; //! Horizontal Tab
	char CtrlLF  = 10  ; //! Line Feed (*NIX Newline -- MOVe printer down a line)
	char CtrlVT  = 11  ; //! Vertical Tab (skip a couple lines)
	char CtrlFF  = 12  ; //! Form Feed (Eject current page -- aka page break)
	char CtrlCR  = 13  ; //! Carriage Return (macOS Newline -- return print to start of line)
	char CtrlSO  = 14  ; //! Shift OUT (to different character set)
	char CtrlSI  = 15  ; //! Shift IN (to original character set)
	char CtrlDLE = 16  ; //! Data Link Escape (interpret next sequence to be different)
	char CtrlDC1 = 17  ; //! Device Control 1
	char CtrlDC2 = 18  ; //! Device Control 2
	char CtrlDC3 = 19  ; //! Device Control 3
	char CtrlDC4 = 20  ; //! Device Control 4
	char CtrlNAK = 21  ; //! Negative Acknowledge (error status acknowledge)
	char CtrlSYN = 22  ; //! Synchronous Idle (correct sync)
	char CtrlETB = 23  ; //! End of Transmission Block (end of paragraph or block)
	char CtrlCAN = 24  ; //! Cancel
	char CtrlEM  = 25  ; //! End of Medium (Usable portion of writing gone) or EM space (#-pt font --> #-char EM space)
	char CtrlSUB = 26  ; //! Substitute 
	char CtrlESC = 27  ; //! Escape (triggers escape sequence if seen in text, sent by ESC key)
	char CtrlFS  = 28  ; //! File Separator
	char CtrlGS  = 29  ; //! Group Separator
	char CtrlRS  = 30  ; //! Record Seperator
	char CtrlUS  = 31  ; //! Unit Separator
	char CtrlSP  = 32  ; //! Space
	char CtrlDEL = 127 ; //! Delete Character (character is deleted, NOT the 'DEL' key)
}

namespace Clib {
// Networking and System Headers
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h> // for nonblocking mode
#include <unistd.h> // for closing descriptors, apparently, as well as sleep
#include <sys/time.h>
}

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
	virtual const char* what() const noexcept;
	virtual const ErrorLevel getLevel() const noexcept;
	virtual const std::string fullWhat() const noexcept;
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
	const Clib::sockaddr_in makeCSocket(u16 i_port) const noexcept;
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
	Clib::sockaddr_in mv_cSocket; //! The "C Socket" representation of the port and IPv4Addr.
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
	
	Clib::sockaddr_in m_cSocket;
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
