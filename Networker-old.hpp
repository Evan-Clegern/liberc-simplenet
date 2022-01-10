#ifndef ERCLIB_Networker
#define ERCLIB_Networker

#include <cstdlib>
#include <iostream>
#include <ctime>
#include <string>
#include <array>
#include <vector>
#include <cstdint>
#include <cerrno>

namespace ERCLIB { namespace Net {

// Networking Headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> // for nonblocking mode

typedef std::uint_fast32_t u32;
typedef std::uint_fast16_t u16;
typedef std::uint_fast64_t u64;
typedef std::uint_fast8_t  u8;


/********!
 * @name ConnType
 * 
 * @brief
 * 			When establishing a BaseSocket, defines which protocol to use.
 * 
 ********/
typedef enum : bool {
	CONNECT_TCP = 1, //! Implement Transmission Control Protocol and enable data streaming.
	CONNECT_UDP = 0  //! Implement User Datagram Protocol.
} ConnType;

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
class c_NetError {
	std::string m_about;
	ErrorLevel m_severity;
public:
	explicit c_NetError(const char* text, const ErrorLevel level);
	explicit c_NetError(const std::string text, const ErrorLevel level);
	const std::string what() const noexcept;
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

//! Class which handles the initialization and storage of a system-level socket and address.
struct c_BaseSocket {
	//The m_ items are not *technically* private, but intended as such.
	
	ConnType m_sockType; //! Whether we're in TCP or UDP.
	bool inUse = 0; //! Intened to prevent a BaseSocket item from being used by multiple IOSocket items at one time (but can be released from one and reused)
	bool m_bound = 0; //! Whether we're bound to our specific port or not; sanity check.
	int m_socketDescriptor; //! Automatically-assigned file descriptor integer for our socket.
	c_IPv4Addr m_address; //! Our IPv4 address we've given ourselves; hopefully just the local one.
	u16 m_socketPort; //! The port we've bound ourselves to.
	sockaddr_in m_cSocket; //! The "C Socket" representation of the port and IPv4Addr.

	//! Initializes the system-level socket with a specified address, port, and connection type.
	explicit c_BaseSocket(c_IPv4Addr i_addr, u16 i_port, ConnType i_type = CONNECT_TCP);
	//! Shuts down and de-registers our socket.
	~c_BaseSocket();
};

//! Class which handles input and output operations atop of a BaseSocket.
class c_IOSocket {
	c_BaseSocket& m_Socket; //Direct reference to our BaseSocket that we're using.
	int m_operateSocket; //! The specific socket file descriptor in use; TCP Listen mode creates its own, but the rest mirror that of BaseSocket
	bool m_socketized = 0; //! Whether we're actually set up or not; sanity check.
	bool m_waitForCnt; //! Whether or not we are in BLOCKING (wait) mode.
	ConnType m_TCP; //! Simplifies (this->m_Socket.m_sockType) into (this->m_TCP)
	TCPSetup m_TCPInp; //! Whether we intend to be in TCP_LISTEN or TCP_CONNECT (if it's even TCP mode)
	int m_oldFlags; //! For restoring the flags of our BaseSocket when we're done
public:
	explicit c_IOSocket(c_BaseSocket& i_sock, ConnWait i_connectWait, TCPSetup i_TCPListen = TCP_SETUP_LISTEN);
	~c_IOSocket();
	
	/********!
	 * @name acceptConnectionTCP
	 * 
	 * @brief
	 * 			Attempts to accept a new inbound TCP connection.
	 * 
	 * @returns
	 * 			TRUE if a connection is established; FALSE if not.
	 ********/
	bool acceptConnectionTCP();
	/********!
	 * @name createConnectionTCP
	 * 
	 * @brief
	 * 			Attempts to create a new outbound TCP connection to a specified host.
	 * 
	 * @param [in] i_client
	 * 			The "C Socket" client to connect to.
	 * 
	 * @returns
	 * 			TRUE if a connection is established; FALSE if not.
	 ********/
	bool createConnectionTCP(sockaddr_in i_client);
	/********!
	 * @name createConnectionTCP
	 * 
	 * @brief
	 * 			Attempts to create a new outbound TCP connection to a specified host.
	 * 
	 * @param [in] i_client
	 * 			The IPv4 Address of our client.
	 * @param [in] i_client
	 * 			The port we want to connect to on the client.
	 * 
	 * @returns
	 * 			TRUE if a connection is established; FALSE if not.
	 ********/
	bool createConnectionTCP(c_IPv4Addr i_client,u16 i_port);
	/********!
	 * @name copyPeerAddrTCP
	 * 
	 * @brief
	 * 			Copies the address and port of our peer connection to specified
	 * 			IPv4Addr and u16 number.
	 * 
	 * @returns
	 * 			TRUE if the address was copied successfully; FALSE if not (i.e. not connected, or in UDP)
	 ********/
	bool copyPeerAddrTCP(c_IPv4Addr* ip_storeMain, u16* ip_storePort);
	/********!
	 * @name receive
	 * 
	 * @brief
	 * 			Attempts to copy over the next set of received data to a specified buffer.
	 * 
	 * @param [in] i_bufferItem
	 * 			Main character buffer to copy to.
	 * @param [in] i_buffSize
	 * 			Size of specified character buffer.
	 * 
	 * @returns
	 * 			TRUE if the internal operation was successful; FALSE if no critical errors occurred (but op was unsuccessful) or if nothing has been received.
	 ********/
	bool receive(char* i_bufferItem, u32 i_buffSize);
	/********!
	 * @name receiveAddrUDP
	 * 
	 * @brief
	 * 			Attempts to copy over the next set of received data to a specified buffer, along with the address and
	 * 			port of the sender. ONLY WORKS FOR UDP MODE.
	 * 
	 * @param [in] i_bufferItem
	 * 			Main character buffer to copy to.
	 * @param [in] i_buffSize
	 * 			Size of specified character buffer.
	 * @param [in] ip_storeDataFrom
	 * 			The IPv4Addr to copy the inbound message's sender data to.
	 * @param [in] ip_storePortFrom
	 * 			The u16 to copy the inbound message's port data to.
	 * 
	 * @returns
	 * 			TRUE if the internal operation was successful; FALSE if no critical errors occurred (but op was unsuccessful) or if nothing has been received.
	 ********/
	bool receiveAddrUDP(char* i_bufferItem, u32 i_buffSize, c_IPv4Addr* ip_storeDataFrom, u16* ip_storePortFrom);
	
	/********!
	 * @name transmitTCP
	 * 
	 * @brief
	 * 			Transmits a specified character buffer over an established TCP stream, optionally with
	 * 			an "END OF RECORD" flag.
	 * 
	 * @param [in] i_bufferItem
	 * 			Main character buffer to copy from.
	 * @param [in] i_bufferSize
	 * 			Size of the specified message buffer.
	 * @param [in] i_last
	 * 			Whether or not to raise the MSG_EOR (End of Record) flag.
	 * 
	 * @returns
	 * 			The number of bytes successfully transmitted.
	 ********/
	u32 transmitTCP(char* i_bufferItem, u32 i_bufferSize, bool i_last = 0);
	/********!
	 * @name transmitUDP
	 * 
	 * @brief
	 * 			Transmits a specified character buffer via UDP to a specified "C Socket"
	 * 			itme, optionally with an "END OF RECORD" flag.
	 * 
	 * @param [in] i_bufferItem
	 * 			Main character buffer to copy from.
	 * @param [in] i_bufferSize
	 * 			Size of the specified message buffer.
	 * @param [in] i_recipient
	 * 			The "C Socket" address to send the UDP datagram to.
	 * @param [in] i_last
	 * 			Whether or not to raise the MSG_EOR (End of Record) flag.
	 * 
	 * @returns
	 * 			The number of bytes successfully transmitted.
	 ********/
	u32 transmitUDP(char* i_bufferItem, u32 i_bufferSize, sockaddr* i_recipient, bool i_last = 0) ;
	/********!
	 * @name transmitUDP
	 * 
	 * @brief
	 * 			Transmits a specified character buffer via UDP to a specified "C Socket"
	 * 			itme, optionally with an "END OF RECORD" flag.
	 * 
	 * @param [in] i_bufferItem
	 * 			Main character buffer to copy from.
	 * @param [in] i_bufferSize
	 * 			Size of the specified message buffer.
	 * @param [in] i_recipient
	 * 			The IPv4Addr of the intended recipient
	 * @param [in] i_portRec
	 * 			The port on the recipient of which we want to send the message to.
	 * @param [in] i_last
	 * 			Whether or not to raise the MSG_EOR (End of Record) flag.
	 * 
	 * @returns
	 * 			The number of bytes successfully transmitted.
	 ********/
	u32 transmitUDP(char* i_bufferItem, u32 i_bufferSize, c_IPv4Addr i_recipient, u16 i_portRec, bool i_last = 0) ;
};

}
}

#endif
