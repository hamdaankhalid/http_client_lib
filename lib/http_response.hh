#ifndef HTTP_MESSAGE_HH
#define HTTP_MESSAGE_HH

#include "http_header.hh"
#include "http_utils.hh"

#include <optional>
#include <string>
#include <vector>

class HttpResponse {
public:
  static std::unique_ptr<HttpResponse>
  FromRawResp(std::vector<unsigned char> &rawResp);

  HttpResponse(std::string httpVersion, int status, std::string reason,
               std::vector<HttpHeader> headers,
               std::vector<unsigned char> body);

  const std::string &GetHTTPVersion() const;
  int GetStatusCode() const;
  const std::string &GetReasonPhrase() const;
  const std::vector<HttpHeader> &GetHeaders() const;
  const std::vector<unsigned char> &GetRawBody() const;
  const HttpHeader *GetHeader(const std::string &key) const;

private:
  std::string m_httpVersion;
  int m_status;
  std::string m_reason;
  std::vector<HttpHeader> m_headers;
  std::vector<unsigned char> m_body;
};

#endif
