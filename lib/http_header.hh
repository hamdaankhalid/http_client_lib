#ifndef HTTP_HEADER
#define HTTP_HEADER

#include <string>

// Simple data type to represent an HTTP header

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

#endif
