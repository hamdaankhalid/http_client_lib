#ifndef HTTP_CLIENT
#define HTTP_CLIENT

#include <iostream>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <vector>

const int HTTP_PORT = 80;
const int HTTPS_PORT = 443;

const char CRLF[] = "\r\n";

const char DELIMITTER = ' ';

enum HTTP_METHOD {
  GET = 0,
  POST = 1,
  PUT = 2,
  DELETE = 3,
};

const char *HTTP_METHOD_STR[] = {"GET", "POST", "PUT", "PUT", "DELETE"};

class HttpHeader {
public:
  HttpHeader(std::string key, std::string val);
  std::string GetRepr() const;

private:
  std::string m_key;
  std::string m_value;
};

class HttpResponse {};

/*
INTERNAL:
 A "message" consists of the following:
control data to describe and route the message,
a headers lookup table of name/value pairs for extending that control data and
conveying additional information about the sender, message, content, or context,
a potentially unbounded stream of content, and
a trailers lookup table of name/value pairs for communicating information
obtained while sending the content.
*/
class HTTPMessage {
public:
  HTTPMessage(HTTP_METHOD method, std::string route, std::string version,
              std::vector<unsigned char> body, std::vector<HttpHeader> headers);

  std::unique_ptr<std::vector<unsigned char>> GetBytes();

private:
  HTTP_METHOD m_method;
  std::string m_route;
  std::string m_httpVersion;
  std::vector<unsigned char> m_body;
  std::vector<HttpHeader> m_headers;
};

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
  // Delete default constructor
  HTTPConnection() = delete;

  // Delete copy constructor and copy assignment since this class
  // handles network resources we don't want leaking on copying
  HTTPConnection(const HTTPConnection &) = delete;
  HTTPConnection &operator=(const HTTPConnection &) = delete;
  // Delete move constructor and move assignment
  HTTPConnection(HTTPConnection &&) = delete;
  HTTPConnection &operator=(HTTPConnection &&) = delete;

  /*
   An HTTPConnection instance represents one transaction with an HTTP server.
   It should be instantiated by passing it a host and optional port number.
   If no port number is passed, the port is extracted from the host string
   if it has the form host:port, else the default HTTP port (80) is used.
   If the optional timeout parameter is given, blocking operations (like
   connection attempts) will timeout after that many seconds (if it is
   not given, the global default timeout setting is used).
   The optional blocksize parameter sets the buffer size in bytes for sending a
   file-like message body.
   */

  HTTPConnection(std::string host, int port, int blockSize);

  /*
  This will send a request to the server using the HTTP request method method
  and the request URI url. The provided url must be an absolute path to conform
  with RFC 2616 §5.1.2 (unless connecting to an HTTP proxy server or using the
  OPTIONS or CONNECT methods).

  If body is specified, the specified data is sent after the headers are
  finished. It may be a str, a bytes-like object, an open file object, or an
  iterable of bytes. If body is a string, it is encoded as ISO-8859-1, the
  default for HTTP. If it is a bytes-like object, the bytes are sent as is. If
  it is a file object, the contents of the file is sent; this file object should
  support at least the read() method. If the file object is an instance of
  io.TextIOBase, the data returned by the read() method will be encoded as
  ISO-8859-1, otherwise the data returned by read() is sent as is. If body is an
  iterable, the elements of the iterable are sent as is until the iterable is
  exhausted.

  The headers argument should be a mapping of extra HTTP headers to send with
  the request. A Host header must be provided to conform with RFC 2616 §5.1.2
  (unless connecting to an HTTP proxy server or using the OPTIONS or CONNECT
  methods).

  If headers contains neither Content-Length nor Transfer-Encoding, but there is
  a request body, one of those header fields will be added automatically. If
  body is None, the Content-Length header is set to 0 for methods that expect a
  body (PUT, POST, and PATCH). If body is a string or a bytes-like object that
  is not also a file, the Content-Length header is set to its length. Any other
  type of body (files and iterables in general) will be chunk-encoded, and the
  Transfer-Encoding header will automatically be set instead of Content-Length.

  The encode_chunked argument is only relevant if Transfer-Encoding is specified
  in headers. If encode_chunked is False, the HTTPConnection object assumes that
  all encoding is handled by the calling code. If it is True, the body will be
  chunk-encoded.
  */
  bool Request(HTTP_METHOD method, const std::string &url,
               const std::vector<unsigned char> &body,
               const std::vector<HttpHeader> &headers);

private:
  std::string host;
  int port;
  int blockSize;
  std::unique_ptr<int, SocketDeleter> m_socketSession;
  std::unique_ptr<struct addrinfo, AddrinfoDeleter> m_resolvedAddr;

  bool establishOrReuseTcpSession();
};

#endif
