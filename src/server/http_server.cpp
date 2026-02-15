#include "http_server.h"
#include "../http/http_parser.h"
#include <iostream>
#include <thread>
#include <netinet/in.h>
#include <sys/socket.h>

HttpServer::HttpServer(int port, const std::string& directory) : port(port), router(directory) {}

void HttpServer::start() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 4221\n";
        return;
    }

    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return;
    }

    std::cout << "Waiting for a client to connect...\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);

        int client_socket = accept(server_fd, (sockaddr *) &client_addr, & client_addr_len);
        if (client_socket < 0) {
            std::cerr << "accept failed\n";
            continue;
        }

        std::thread(&HttpServer::handle_client, this, client_socket).detach();
    }
}

void HttpServer::handle_client(int client_fd) {
    char buffer[4096] = {0};
    std::string stream_buffer;

    while (true) {
        while (stream_buffer.find("\r\n\r\n") == std::string::npos) {
            int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                close(client_fd);
                return;
            }
            stream_buffer.append(buffer, bytes);
        }

        size_t header_end_pos = stream_buffer.find("\r\n\r\n");
        std::string header_part = stream_buffer.substr(0, header_end_pos + 4);

        HttpRequest temp_req = HttpParser::parse(header_part);

        size_t content_length = 0;
        std::string len_str = temp_req.get_header("Content-Length");
        if (!len_str.empty())
            content_length = std::stoul(len_str);

        size_t total_request_size = header_end_pos + 4 + content_length;

        while (stream_buffer.size() < total_request_size) {
            int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                close(client_fd);
                return;
            }
            stream_buffer.append(buffer, bytes);
        }

        std::string full_request_raw = stream_buffer.substr(0, total_request_size);
        stream_buffer.erase(0, total_request_size);

        HttpRequest req = HttpParser::parse(full_request_raw);
        HttpResponse res = router.route(req);

        if (req.get_header("Connection") != "close") {
            res.set_header("Connection", "keep-alive");
        }

        std::string response = res.to_string();
        send(client_fd, response.c_str(), response.length(), 0);

        if (req.get_header("Connection") == "close") break;
    }
    close(client_fd);
}