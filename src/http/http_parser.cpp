#include "http_parser.h"
#include <sstream>

HttpRequest HttpParser::parse(const std::string& raw) {
    HttpRequest request;
    std::istringstream stream(raw);
    std::string line;

    std::getline(stream, line);
    std::istringstream request_line(line);
    request_line >> request.method
                 >> request.path
                 >> request.version;

    while (std::getline(stream, line) && line != "\r") {
        auto colon = line.find(":");
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);
            if (!value.empty() && value.back() == '\r')
                value.pop_back();
            request.headers[key] = value;
        }
    }

    return request;
}
