
#ifndef HTTP_SERVER_STARTER_CPP_ROUTER_H
#define HTTP_SERVER_STARTER_CPP_ROUTER_H
#include "../http/http_request.h"
#include "../http/http_response.h"
#include "../services/file_service.h"
#include <string>

class Router {
public:
    explicit Router(const std::string& directory);
    HttpResponse route(const HttpRequest& request);

private:
    FileService file_service;
};
#endif //HTTP_SERVER_STARTER_CPP_ROUTER_H