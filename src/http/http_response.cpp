#include "http_response.h"
#include <sstream>

HttpResponse::HttpResponse(int status_code) {
    set_status_code(status_code);
}

void HttpResponse::set_status_code(int code) {
    status_code = code;
    status_message = get_status_message_by_status_code(code);
}

void HttpResponse::set_header(const std::string& key,
                              const std::string& value) {
    headers[key] = value;
}

void HttpResponse::set_body(const std::string& b) {
    body = b;
    headers["Content-Length"] = std::to_string(body.size());
}

std::string HttpResponse::to_string() const {
    std::ostringstream response;

    response << "HTTP/1.1 "
        << status_code << " "
        << status_message << "\r\n";

    for (const auto& [key, value] : headers) {
        response << key << ": " << value << "\r\n";
    }

    response << "\r\n";
    response << body;

    return response.str();
}

std::string HttpResponse::get_status_message_by_status_code(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 404: return "Not Found";
        case 201: return "Created";
        case 400: return "Bad Request";
        default: return "Undefined";
    }
}
