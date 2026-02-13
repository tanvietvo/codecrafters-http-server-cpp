#ifndef CODECRAFTERS_HTTP_SERVER_CPP_HTTP_REQUEST_H
#define CODECRAFTERS_HTTP_SERVER_CPP_HTTP_REQUEST_H
#include <string>
#include <unordered_map>

struct HttpRequest {
    public:
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string get_header(const std::string& key) const {
        auto it = headers.find(key);
        return (it != headers.end()) ? it->second : "";
    }
};
#endif //CODECRAFTERS_HTTP_SERVER_CPP_HTTP_REQUEST_H