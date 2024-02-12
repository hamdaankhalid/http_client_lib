#ifndef HTTP_MESSAGE_HH
#define HTTP_MESSAGE_HH

#include <optional>
#include <string>
#include <vector>

const char DELIMITTER = ' ';

enum HTTP_METHOD {
  GET = 0,
  POST = 1,
  PUT = 2,
  DELETE = 3,
};

extern const char *HTTP_METHOD_STR[];

class HttpHeader {
public:
  HttpHeader(std::string key, std::string val);
  std::string GetRepr() const;
  const std::string &GetKey() const;
  const std::string &GetValue() const;

private:
  std::string m_key;
  std::string m_value;
};

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
