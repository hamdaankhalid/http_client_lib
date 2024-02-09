#include "http_client.hh"

#include <c++/v1/__nullptr>
#include <iostream>
#include <iterator>
#include <memory>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

HttpHeader::HttpHeader(std::string key, std::string val)
    : m_key(key), m_value(val) {}

std::string HttpHeader::GetRepr() const { return m_key + ":" + m_value; }

HTTPMessage::HTTPMessage(HTTP_METHOD method, std::string route,
                         std::string version, std::vector<unsigned char> body,
                         std::vector<HttpHeader> headers)
    : m_method(method), m_route(route), m_httpVersion(version), m_body(body),
      m_headers(headers) {}

/*
 *  HTTP-message   = start-line CRLF
                   *( field-line CRLF )
                   CRLF
                   [ message-body ]
 */
std::unique_ptr<std::vector<unsigned char>> HTTPMessage::GetBytes() {
  std::unique_ptr<std::vector<unsigned char>> data =
      std::make_unique<std::vector<unsigned char>>();

  // method
  std::string method(HTTP_METHOD_STR[m_method]);
  std::copy_n(method.c_str(), method.size(), std::back_inserter(*data));
  data->push_back(DELIMITTER);

  // route
  std::copy_n(m_route.c_str(), m_route.size(), std::back_inserter(*data));
  data->push_back(DELIMITTER);

  // HTTP Version
  std::copy_n(m_httpVersion.c_str(), m_httpVersion.size(),
              std::back_inserter(*data));

  // start line ends with CRLF
  std::copy_n(CRLF, 2, std::back_inserter(*data));

  // field-line and CRLF per line
  for (const HttpHeader &header : m_headers) {
    std::string headerData = header.GetRepr() + CRLF;
    std::copy_n(headerData.c_str(), headerData.size(),
                std::back_inserter(*data));
  }

  std::copy_n(CRLF, 2, std::back_inserter(*data));

  // GET, HEAD, OPTIONS, TRACE DO NOT HAVE BODY
  // For now we only support we are not doing HEAD and OPTIONS
  // Optional message body
  if (m_method != HTTP_METHOD::GET) {
    std::copy_n(m_body.data(), m_body.size(), std::back_inserter(*data));
  }

  // Print the converted string
  std::string str(data->begin(), data->end());
  std::cout << "Converted string: \n" << str << std::endl;

  return data;
}

void printVector(const std::vector<unsigned char> &vec, ssize_t limit) {
  std::cout << std::string(vec.begin(), vec.begin() + limit) << std::endl;
}

/* Connection:
 * A transport layer virtual circuit established between two
 * application programs for the purpose of communication.
 * */
HTTPConnection::HTTPConnection(std::string host, int port, int blockSize)
    : host(host), port(port), blockSize(blockSize), m_socketSession(nullptr),
      m_resolvedAddr(nullptr) {}

bool HTTPConnection::Request(HTTP_METHOD method, const std::string &url,
                             const std::vector<unsigned char> &body,
                             const std::vector<HttpHeader> &headers) {
  if (!this->establishOrReuseTcpSession()) {
    return false;
  }

  HTTPMessage request(method, url, "HTTP/1.1", body, headers);
  std::unique_ptr<std::vector<unsigned char>> data = request.GetBytes();

  ssize_t totalSent = 0;
  while (totalSent < data->size()) {
    ssize_t sent = send(*m_socketSession.get(), data->data() + totalSent,
                        data->size() - totalSent, 0);
    if (sent < 0) {
      std::cout << "Couldn't send!" << std::endl;
      return false;
    }
    totalSent += sent;
  }

  // read response
  std::vector<unsigned char> readBuffer(367);
  ssize_t totalRecvd = 0;
  bool readFurther = true;
  while (readFurther) {
    ssize_t readIn =
        recv(*m_socketSession.get(), readBuffer.data() + totalRecvd, 367, 0);
    if (readIn == 0) {
      readFurther = false;
    }

    if (readIn < 0) {
      std::cout << "Couldn't get response!" << std::endl;
      return false;
    }
    totalRecvd += readIn;
  }
  
  // TODO: parse response 
  std::cout << "RESP " << totalRecvd << std::endl;
  printVector(readBuffer, totalRecvd);

  // TODO: based on response clear our tcp socket
  m_socketSession = nullptr;

  return true;
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
