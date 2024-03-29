#include "http_connection.hh"

#include <iostream>
#include <iterator>
#include <memory>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// Custom deleter so we can use socket with unique_ptr
void SocketDeleter::operator()(int *socket) const {
  if (socket != nullptr && *socket >= 0) {
    close(*socket);
  }
  delete socket; // Don't forget to release the memory
}

// AddrinfoDeleter so we can use struct addrinfo wrapped with unique_ptr
void AddrinfoDeleter::operator()(addrinfo *p) const {
  if (p != nullptr) {
    freeaddrinfo(p);
  }
}

/* Connection:
 * A transport layer virtual circuit established between two
 * application programs for the purpose of communication.
 * */
HTTPConnection::HTTPConnection(std::string host, int port, int blockSize)
    : host(host), port(port), blockSize(blockSize), m_socketSession(nullptr),
      m_resolvedAddr(nullptr) {}

std::unique_ptr<HttpResponse>
HTTPConnection::Request(HTTP_METHOD method, const std::string &url,
                        const std::vector<unsigned char> &body,
                        const std::vector<HttpHeader> &headers) {
  if (!this->establishOrReuseTcpSession()) {
    return nullptr;
  }

  // TODO: make http version configurable
  HTTPRequest request(method, url, "HTTP/1.1", body, headers);
  std::vector<unsigned char> data = request.GetBytes();

  ssize_t totalSent = 0;
  while (totalSent < data.size()) {
    ssize_t sent = send(*m_socketSession.get(), data.data() + totalSent,
                        data.size() - totalSent, 0);
    if (sent < 0) {
      std::cout << "Couldn't send!" << std::endl;
      return nullptr;
    }
    totalSent += sent;
  }

  // read response
  std::vector<unsigned char> readBuffer(1024);
  ssize_t totalRecvd = 0;
  bool readFurther = true;
  while (readFurther) {
    ssize_t readIn = recv(*m_socketSession.get(),
                          readBuffer.data() + totalRecvd, readBuffer.size(), 0);
    if (readIn == 0) {
      readFurther = false;
    }

    if (readIn < 0) {
      std::cout << "Couldn't get response!" << std::endl;
      return nullptr;
    }
    totalRecvd += readIn;
  }

  // truncate this bad boy
  readBuffer.resize(totalRecvd);
  std::unique_ptr<HttpResponse> resp = HttpResponse::FromRawResp(readBuffer);

  // conditionally close tcp connection
  const HttpHeader *connHeader = resp->GetHeader("Connection");
  if (connHeader != nullptr && connHeader->GetValue() == "close") {
    m_socketSession = nullptr;
  }

  return resp;
}

bool HTTPConnection::establishOrReuseTcpSession() {
  if (m_socketSession != nullptr) {
    return true;
  }

  int createdSocket;
  // Connect a socket
  if ((createdSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cout << "Failed to create socket: "
              << std::string(std::strerror(errno)) << std::endl;
    return false;
  }

  m_socketSession = std::unique_ptr<int, SocketDeleter>(new int(createdSocket));

  // Host name resolution from string host to IP addr
  // This step should be avoided if establishing again and again
  struct addrinfo resolutionHints;
  memset(&resolutionHints, 0, sizeof(resolutionHints));
  resolutionHints.ai_family = AF_INET;       // IPv4
  resolutionHints.ai_socktype = SOCK_STREAM; // TCP

  struct addrinfo *resolvedAddr;
  if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &resolutionHints,
                  &resolvedAddr) < 0) {
    std::cout << "Failed to resolve host address: " +
                     std::string(std::strerror(errno))
              << std::endl;
    return false;
  }

  m_resolvedAddr =
      std::unique_ptr<struct addrinfo, AddrinfoDeleter>(resolvedAddr);

  bool connected = false;
  struct addrinfo *candidateAddr = m_resolvedAddr.get();

  while (candidateAddr != nullptr) {
    // Attempt connection
    if (connect(*m_socketSession, candidateAddr->ai_addr,
                candidateAddr->ai_addrlen) == 0) {
      // Socket connected to server, we don't need to hold data for addr
      connected = true;
      break;
    }
    candidateAddr = candidateAddr->ai_next;
  }

  // No socket connected
  if (!connected) {
    std::cout << "failed to connect in dns resolution" << std::endl;
    m_socketSession = nullptr;
    return false;
  }

  return true;
}
