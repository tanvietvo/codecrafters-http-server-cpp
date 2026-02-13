#ifndef CODECRAFTERS_HTTP_SERVER_CPP_HTTP_RESPONSE_H
#define CODECRAFTERS_HTTP_SERVER_CPP_HTTP_RESPONSE_H
#include <string>
#include <unordered_map>

class HttpResponse {
    public:
    HttpResponse(int status_code);
    void set_status_code(int status_code);
    void set_header(const std::string& key, const std::string& value);
    void set_body(const std::string& body);
    std::string to_string() const;

    private:
    int status_code;
    std::string status_message;
    std::string get_status_message_by_status_code(int status_code);
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};
#endif //CODECRAFTERS_HTTP_SERVER_CPP_HTTP_RESPONSE_H