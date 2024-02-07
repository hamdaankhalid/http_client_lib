#include "http_client.h"

#include <cstring>
#include <stdexcept>
#include <sys/socket.h>

/*
 * Http/1.0
 * https://datatracker.ietf.org/doc/html/rfc1945#section-1.1
 */

/* Connection:
 * A transport layer virtual circuit established between two
 * application programs for the purpose of communication.
 * */
HTTPConnection::HTTPConnection(std::string host, int port = 80, int blockSize = 8192) : host(host), port(port), blockSize(blockSize) {
	// connect a socket		
	if ((m_socketSession = socket(PF_INET | PF_INET6, SOCK_STREAM, 0)) < 0) {
			throw std::runtime_error("Failed to create socket: " + std::string(std::strerror(errno)));
	}
	
	// host name resolution from string host to IP addr
	struct addrinfo *resolvedAddr;
	struct addrinfo resolutionHints;
	memset(&resolutionHints, 0, sizeof(resolutionHints));

	if ((getaddrinfo(host, port, &resolutionHints, &resolvedAddr) < 0)) {
			close(m_socketSession);
	}
	
	bool connected = false;
	struct addrinfo *candidateAddr;
	candidateAddr = resolvedAddr;
	while (candidateAddr != NULL) {
		// attempt connection
		if (connect(m_socketSession, candidateAddr->ai_addr, candidateAddr->ai_addrlen) == 0) {
			// socket connected to server, we don't need to hold data for addr
			connected = true;
			break;
		}
		
		candidateAddr = candidateAddr->ai_next;
	}

	// no socket connected
	freeaddrinfo(resolvedAddr);

	if (!connected) {
		close(m_socketSession);
		throw std::runtime_error("Failed to establish TCP session");
	}
	// at this point TCP socket has connected and can be used for requests
}
