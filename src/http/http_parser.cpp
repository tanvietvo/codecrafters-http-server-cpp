#include "http_parser.h"
#include <sstream>

HttpRequest HttpParser::parse(const std::string& raw) {
    HttpRequest request;

    size_t header_end = raw.find("\r\n\r\n");
    std::string body_part;
    if (header_end != std::string::npos) {
        body_part = raw.substr(header_end + 4);
    }

    std::istringstream stream(raw);
    std::string line;

    std::getline(stream, line);
    std::istringstream request_line(line);
    request_line >> request.method
                 >> request.path
                 >> request.version;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        auto colon = line.find(":");
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);
            request.headers[key] = value;
        }
    }

    std::string content_length = request.get_header("Content-Length");
    if (!content_length.empty()) {
        size_t length = atoi(content_length.c_str());
        request.body = body_part.substr(0, length);
    }

    return request;
}
