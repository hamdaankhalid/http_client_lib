#ifndef HTTP_REQUEST
#define HTTP_REQUEST

#include "http_utils.hh"
#include "http_header.hh"

#include <vector>
#include <string>

class HTTPRequest {
public:
  HTTPRequest(HTTP_METHOD method, std::string route, std::string version,
              std::vector<unsigned char> body, std::vector<HttpHeader> headers);

  std::vector<unsigned char> GetBytes();

private:
  HTTP_METHOD m_method;
  std::string m_route;
  std::string m_httpVersion;
  std::vector<unsigned char> m_body;
  std::vector<HttpHeader> m_headers;
};

#endif
