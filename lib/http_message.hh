#ifndef HTTP_MESSAGE_HH
#define HTTP_MESSAGE_HH

#include <string>
#include <vector>

const char CRLF[] = "\r\n";

const char DELIMITTER = ' ';

enum HTTP_METHOD {
  GET = 0,
  POST = 1,
  PUT = 2,
  DELETE = 3,
};

const char* HTTP_METHOD_STR[] = {"GET", "POST", "PUT", "PUT", "DELETE"};

class HttpHeader {
public:
  HttpHeader(std::string key, std::string val);
  std::string GetRepr() const;

private:
  std::string m_key;
  std::string m_value;
};

// TODO: Need a way to have it parsed into, need getters
class HttpResponse {};

class HTTPMessage {
public:
  HTTPMessage(HTTP_METHOD method, std::string route, std::string version,
              std::vector<unsigned char> body, std::vector<HttpHeader> headers);

  std::unique_ptr<std::vector<unsigned char> > GetBytes();

private:
  HTTP_METHOD m_method;
  std::string m_route;
  std::string m_httpVersion;
  std::vector<unsigned char> m_body;
  std::vector<HttpHeader> m_headers;
};

#endif
