#include "http_message.hh"

#include <__nullptr>
#include <optional>
#include <iostream>
#include <memory>
#include <regex>
#include <vector>

const char *HTTP_METHOD_STR[] = {"GET", "POST", "PUT", "PUT", "DELETE"};

HttpHeader::HttpHeader(std::string key, std::string val)
    : m_key(key), m_value(val) {}

std::string HttpHeader::GetRepr() const { return m_key + ":" + m_value; }

const std::string& HttpHeader::GetKey() const {
	return m_key;
}

const std::string& HttpHeader::GetValue() const {
	return m_value;
}

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

// omit and give the metadata char block, the user can then use the metadata
// block to get raw resp omission signal is purely the existence of 2
// consecutive CRLFs
int omitBody(std::vector<unsigned char> &raw) {
  int lastConsecutiveCRLFAt = -1;
  int consecutiveCRLFS = 0;
  // because we use the lookahead operation we just traverse 1 less than end
  for (int i = 0; i < raw.size() - 1; i++) {
    char curr = raw[i];
    char lookahead = raw[i + 1];
    std::string candidate;
    candidate += curr;
    candidate += lookahead;
    if (candidate == CRLF) {
      // maybe second one?
      if (consecutiveCRLFS == 1) {
        return i;
      }
      // maybe first one?
      consecutiveCRLFS = 1;
      lastConsecutiveCRLFAt = i;
    } else {
      // check if non CRLF breaks the streak;
      // to break the CRLF we need to be sitting away by more than one char?
      if (i != -1 && i - lastConsecutiveCRLFAt > 1) {
        consecutiveCRLFS = 0;
        lastConsecutiveCRLFAt = -1;
      }
    }
  }

  return -1;
}
/*
 * Response = HTTP-version SP status-code SP [ reason-phrase ] CRLF
                           *( field-line CRLF )
                           CRLF
                           [ message-body ]
 * */
std::unique_ptr<HttpResponse>
HttpResponse::FromRawResp(std::vector<unsigned char> &rawResp) {
  // truncate text based metadata for as far as we can, and then use that
  // with string to parse everything except body.
  int metadataBoundary = omitBody(rawResp);
  if (metadataBoundary == -1) {
    std::cout << "Malformed response obj" << std::endl;
    return nullptr;
  }
  std::string response(rawResp.begin(), rawResp.begin() + metadataBoundary);

  // Parse, create and return unq_ptr
  // status line regex
  // Headers regex
  // message-body
  std::regex resp("^"              // start of line
                  "(HTTP/\\d.\\d)" // http version
                  "\\s"
                  "(\\d\\d\\d)" // status code
                  "\\s"
                  "(\\w*)" // reason phrase
                  "\r\n"
                  "((.|\r\n)*)" // raw headers
                  "\r\n");

  std::smatch matchingResult;
  if (!std::regex_search(response, matchingResult, resp)) {
    std::cout << "Unparsable buffer" << std::endl;
    return nullptr;
  }

  std::string httpVersion = matchingResult[1];
  int statusCode = std::stoi(matchingResult[2]);
  std::string reasonPhrase = matchingResult[3];

  // raw to Header format
  std::string rawHeaders = matchingResult[4];
  std::vector<HttpHeader> headers;

  std::regex headerRegex("(.*?):\\s*(.*?)\\s*\r\n");
  std::smatch match;

  std::string::const_iterator searchStart(rawHeaders.cbegin());
  while (
      std::regex_search(searchStart, rawHeaders.cend(), match, headerRegex)) {
    if (match.size() == 3) {
      std::string key = match[1].str();
      std::string value = match[2].str();
	  headers.push_back(HttpHeader(key, value));
    }
    searchStart = match.suffix().first;
  }

  std::vector<unsigned char> rawBody(rawResp.begin() + metadataBoundary,
                                     rawResp.end());

  std::unique_ptr<HttpResponse> parsed = std::make_unique<HttpResponse>(
      httpVersion, statusCode, reasonPhrase, headers, rawBody);

  return parsed;
}

HttpResponse::HttpResponse(std::string httpVersion, int status,
                           std::string reason, std::vector<HttpHeader> headers,
                           std::vector<unsigned char> body)
    : m_httpVersion(httpVersion), m_status(status), m_reason(reason),
      m_headers(headers), m_body(body) {}

const std::string &HttpResponse::GetHTTPVersion() const {
  return m_httpVersion;
}

int HttpResponse::GetStatusCode() const { return m_status; }

const std::string &HttpResponse::GetReasonPhrase() const { return m_reason; }

const std::vector<HttpHeader> &HttpResponse::GetHeaders() const {
  return m_headers;
}

const std::vector<unsigned char> &HttpResponse::GetRawBody() const {
  return m_body;
}

const HttpHeader* HttpResponse::GetHeader(const std::string& key) const {
	for (const HttpHeader& header : m_headers) {
		if (header.GetKey() == key) {
			return &header;
		}
	}
	return nullptr;
}

