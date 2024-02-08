#include "http_client.hh"

#include <memory>
#include <string>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <netdb.h>
#include <vector>
#include <unistd.h>

/*
 * Http/1.0
 * https://datatracker.ietf.org/doc/html/rfc1945#section-1.1
 */

HttpHeader::HttpHeader(std::string key, std::string val)
    : m_key(key), m_value(val) {}

std::string HttpHeader::GetRepr() const { 
	return m_key + ": " + m_value; 
}

HTTPMessage::HTTPMessage(HTTP_METHOD method, std::string route, std::string version,
		std::vector<unsigned char> body, std::vector<HttpHeader> headers)
      : m_method(method), m_route(route), m_httpVersion(version), m_body(body),
        m_headers(headers) {}

std::unique_ptr<std::vector<unsigned char>> HTTPMessage::GetBytes() {
	std::unique_ptr<std::vector<unsigned char>> data = std::make_unique<std::vector<unsigned char>>();
	
	// method
    std::string method(HTTP_METHOD_STR[m_method]);
	data->insert(data->end(), std::begin(method), std::end(method));
    data->push_back(DELIMITTER);

    // route
	data->insert(data->end(), std::begin(m_route), std::end(m_route));
    data->push_back(DELIMITTER);
	
	// HTTP Version
	data->insert(data->end(), std::begin(m_httpVersion), std::end(m_httpVersion));

	// start line ends with CRLF
    data->insert(data->end(), std::begin(CRLF), std::end(CRLF));
	
	// field-line and CRLF per line
    for (const HttpHeader &header : m_headers) {
        std::string headerData = header.GetRepr() + CRLF;
		data->insert(data->end(), std::begin(headerData), std::end(headerData));
    }
	
	data->insert(data->end(), std::begin(CRLF), std::end(CRLF));


	// GET, HEAD, OPTIONS, TRACE DO NOT HAVE BODY
	// For now we only support we are not doing HEAD and OPTIONS
	if (m_method != HTTP_METHOD::GET)
	{
		data->insert(data->end(), std::begin(m_body), std::end(m_body));
	}

    // Print the converted string
	std::string str(data->begin(), data->end());
    std::cout << "Converted string: \n" << str << std::endl;

	return data;
}

/* Connection:
 * A transport layer virtual circuit established between two
 * application programs for the purpose of communication.
 * */
HTTPConnection::HTTPConnection(std::string host, int port, int blockSize)
    : host(host), port(port), blockSize(blockSize), m_socketSession(-1) {
  // Connect a socket
  if ((m_socketSession = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(std::strerror(errno)));
  }

  // Host name resolution from string host to IP addr
  struct addrinfo *resolvedAddr;
  struct addrinfo resolutionHints;
  memset(&resolutionHints, 0, sizeof(resolutionHints));
  resolutionHints.ai_family = AF_INET;  // IPv4
  resolutionHints.ai_socktype = SOCK_STREAM;  // TCP

  if ((getaddrinfo(host.c_str(), std::to_string(port).c_str(), &resolutionHints, &resolvedAddr)) < 0) {
    close(m_socketSession);
    throw std::runtime_error("Failed to resolve host address: " +
                             std::string(std::strerror(errno)));
  }

  bool connected = false;
  struct addrinfo *candidateAddr;
  candidateAddr = resolvedAddr;
  while (candidateAddr != NULL) {
    // Attempt connection
    if (connect(m_socketSession, candidateAddr->ai_addr,
                candidateAddr->ai_addrlen) == 0) {
      // Socket connected to server, we don't need to hold data for addr
      connected = true;
      break;
    }

    candidateAddr = candidateAddr->ai_next;
  }

  // No socket connected
  freeaddrinfo(resolvedAddr);

  if (!connected) {
    close(m_socketSession);
    throw std::runtime_error("Failed to establish TCP session");
  }
  std::cout << "established" << std::endl;
}

void HTTPConnection::Request(HTTP_METHOD method, const std::string &url,
                             const std::vector<unsigned char> &body,
                             const std::vector<HttpHeader> &headers) {

	HTTPMessage request(method, url, "HTTP/1.1", body, headers);

	std::unique_ptr<std::vector<unsigned char>> data = request.GetBytes();

	if (send(m_socketSession, data->data(), data->size(), 0) < 0) {
		std::cout << "Couldn't send!" << std::endl;
	}

	// data is sent, recv response
	std::cout << "Sent!" << std::endl;  
}

