#ifndef CODECRAFTERS_HTTP_SERVER_CPP_HTTP_PARSER_H
#define CODECRAFTERS_HTTP_SERVER_CPP_HTTP_PARSER_H
#include <string>
#include "http_request.h"

class HttpParser {
    public:
    static HttpRequest parse(const std::string& raw);
};
#endif //CODECRAFTERS_HTTP_SERVER_CPP_HTTP_PARSER_H