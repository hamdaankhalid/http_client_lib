#ifndef HTTP_UTILS
#define HTTP_UTILS

enum HTTP_METHOD {
  GET = 0,
  POST = 1,
  PUT = 2,
  DELETE = 3,
};

extern const char *HTTP_METHOD_STR[];

extern const char CRLF[];

extern const char DELIMITTER;

#endif
