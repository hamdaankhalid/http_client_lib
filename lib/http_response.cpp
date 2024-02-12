#include "http_response.hh"

#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <vector>

// forward decl util
int omitBody(std::vector<unsigned char> &raw);


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

  std::regex headerRegex("(.*?):\\s*(.*?)\\s*(\r\n|$)");
  std::smatch match;
  std::string::const_iterator searchStart(rawHeaders.cbegin());
  while (
      std::regex_search(searchStart, rawHeaders.cend(), match, headerRegex)) {
    std::string key = match[1].str();
    std::string value = match[2].str();
    headers.push_back(HttpHeader(key, value));
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

// using const pointer because cannot use nullptr with const ref on return if
// nothing found
const HttpHeader *HttpResponse::GetHeader(const std::string &key) const {
  for (int i = 0; i < m_headers.size(); i++) {
    if (m_headers[i].GetKey() == key) {
      return &m_headers[i];
    }
  }
  return nullptr;
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
