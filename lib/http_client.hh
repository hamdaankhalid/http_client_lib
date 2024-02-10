#ifndef HTTP_CLIENT_HH
#define HTTP_CLIENT_HH

#include "http_message.hh"

#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <vector>

const int HTTP_PORT = 80;
const int HTTPS_PORT = 443;

// Custom deleter so we can use socket with unique_ptr
class SocketDeleter {
public:
  void operator()(int *socket) const {
    if (socket != nullptr && *socket >= 0) {
      close(*socket);
    }
    delete socket; // Don't forget to release the memory
  }
};

// AddrinfoDeleter so we can use struct addrinfo wrapped with unique_ptr
class AddrinfoDeleter {
public:
  void operator()(addrinfo *p) const {
    if (p != nullptr) {
      freeaddrinfo(p);
    }
  }
};

// PUBLIC EXPORT
class HTTPConnection {
public:
  // ---- PUBLICLY EXPORTED FUNCS ----

  HTTPConnection(std::string host, int port, int blockSize);

  bool Request(HTTP_METHOD method, const std::string &url,
               const std::vector<unsigned char> &body,
               const std::vector<HttpHeader> &headers);

  // ---- End of Public Exported Method sections ----
	
  // Delete default constructor
  HTTPConnection() = delete;
  // Delete copy constructor and copy assignment since this class
  // handles network resources we don't want leaking on copying
  HTTPConnection(const HTTPConnection &) = delete;
  HTTPConnection &operator=(const HTTPConnection &) = delete;
  // Delete move constructor and move assignment
  HTTPConnection(HTTPConnection &&) = delete;
  HTTPConnection &operator=(HTTPConnection &&) = delete;

private:
  std::string host;
  int port;
  int blockSize;
  std::unique_ptr<int, SocketDeleter> m_socketSession;
  std::unique_ptr<struct addrinfo, AddrinfoDeleter> m_resolvedAddr;

  bool establishOrReuseTcpSession();
};

#endif
