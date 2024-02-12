#include "http_request.hh"


HTTPRequest::HTTPRequest(HTTP_METHOD method, std::string route,
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
std::vector<unsigned char> HTTPRequest::GetBytes() {

  std::vector<unsigned char> data;

  // method
  std::string method(HTTP_METHOD_STR[m_method]);
  std::copy_n(method.c_str(), method.size(), std::back_inserter(data));
  data.push_back(DELIMITTER);

  // route
  std::copy_n(m_route.c_str(), m_route.size(), std::back_inserter(data));
  data.push_back(DELIMITTER);

  // HTTP Version
  std::copy_n(m_httpVersion.c_str(), m_httpVersion.size(),
              std::back_inserter(data));

  // start line ends with CRLF
  std::copy_n(CRLF, 2, std::back_inserter(data));

  // field-line and CRLF per line
  for (const HttpHeader &header : m_headers) {
    std::string headerData = header.GetRepr() + CRLF;
    std::copy_n(headerData.c_str(), headerData.size(),
                std::back_inserter(data));
  }

  std::copy_n(CRLF, 2, std::back_inserter(data));

  // GET, HEAD, OPTIONS, TRACE DO NOT HAVE BODY
  // For now we only support we are not doing HEAD and OPTIONS
  // Optional message body
  if (m_method != HTTP_METHOD::GET) {
    std::copy_n(m_body.data(), m_body.size(), std::back_inserter(data));
  }

  return data;
}
