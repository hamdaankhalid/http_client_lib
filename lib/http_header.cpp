#include "http_header.hh"

// Simple data type to represent an HTTP header

HttpHeader::HttpHeader(std::string key, std::string val)
    : m_key(key), m_value(val) {}

std::string HttpHeader::GetRepr() const { return m_key + ":" + m_value; }

const std::string &HttpHeader::GetKey() const { return m_key; }

const std::string &HttpHeader::GetValue() const { return m_value; }
