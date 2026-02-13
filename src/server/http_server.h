#ifndef CODECRAFTERS_HTTP_SERVER_CPP_HTTP_SERVER_H
#define CODECRAFTERS_HTTP_SERVER_CPP_HTTP_SERVER_H
#include "../router/router.h"

class HttpServer {
    public:
    HttpServer(int port, const std::string& directory);
    void start();

    private:
    int port;
    Router router;

    void handle_client(int client_fd);
};
#endif //CODECRAFTERS_HTTP_SERVER_CPP_HTTP_SERVER_H