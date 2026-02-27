#ifndef CODECRAFTERS_HTTP_SERVER_CPP_HTTP_SERVER_H
#define CODECRAFTERS_HTTP_SERVER_CPP_HTTP_SERVER_H

#include "../router/router.h"

struct ConnectionState {
    int fd;
    std::string stream_buffer;
};

class HttpServer {
public:
    HttpServer(int port, const std::string& directory);
    void start();

private:
    int port;
    Router router;

    void handle_client(int client_fd) const;
    void handle_client_nonblocking(ConnectionState* conn, int epoll_fd) const;
};
#endif //CODECRAFTERS_HTTP_SERVER_CPP_HTTP_SERVER_H