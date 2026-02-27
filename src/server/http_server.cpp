#include "http_server.h"
#include "../http/http_parser.h"
#include <fcntl.h>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <queue>
#include <thread>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define MAX_EVENTS 1024
#define READ_BUFFER 4096
#define THREADS_IN_POOL 8

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static bool send_all(int fd, const std::string& data) {
    size_t total_sent = 0;
    while (total_sent < data.size()) {
        ssize_t sent = send(fd,
                            data.data() + total_sent,
                            data.size() - total_sent,
                            0);
        if (sent > 0) {
            total_sent += sent;
        } else if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

std::unordered_map<int, ConnectionState> sessions;
std::mutex sessions_mutex;

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    bool stop = false;

public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    void enqueue(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            tasks.emplace(std::move(task));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stop = true;
        }
        condition.notify_all();
        for (auto& w: workers) {
            w.join();
        }
    }
};

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

    // while (true) {
    //     sockaddr_in client_addr{};
    //     socklen_t client_addr_len = sizeof(client_addr);
    //
    //     int client_socket = accept(server_fd, (sockaddr *) &client_addr, & client_addr_len);
    //     if (client_socket < 0) {
    //         std::cerr << "accept failed\n";
    //         continue;
    //     }
    //
    //     std::thread(&HttpServer::handle_client, this, client_socket).detach();
    // }

    set_nonblocking(server_fd);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) return;

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    ThreadPool pool(THREADS_IN_POOL);
    std::vector<epoll_event> events(MAX_EVENTS);

    while (true) {
        int n = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                while (true) {
                    sockaddr_in client_addr{};
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                    if (client_fd == -1) break;

                    set_nonblocking(client_fd);

                    auto* connection = new ConnectionState();
                    connection->fd = client_fd;

                    epoll_event client_event{};
                    client_event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    client_event.data.ptr = connection;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event);
                }
            } else {
                auto* conn = static_cast<ConnectionState*>(events[i].data.ptr);

                pool.enqueue([this, conn, epoll_fd]() {
                    this->handle_client_nonblocking(conn, epoll_fd);
                });
            }
        }
    }
}

//Create 1 thread for each request
void HttpServer::handle_client(int client_fd) const {
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
        } else {
            res.set_header("Connection", "close");
        }

        std::string response = res.to_string();
        send(client_fd, response.c_str(), response.length(), 0);

        if (req.get_header("Connection") == "close") {
            break;
        }
    }
    close(client_fd);
}

//Thread Pool và IO Multiplexing (epoll)
void HttpServer::handle_client_nonblocking(ConnectionState* conn, int epoll_fd) const {
    char buffer[READ_BUFFER];
    bool closed = false;

    while (true) {
        ssize_t bytes = recv(conn->fd, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            conn->stream_buffer.append(buffer, bytes);
        } else if (bytes == 0) {
            closed = true;
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            closed = true;
            break;
        }
    }

    if (!closed) {
        while (true) {
            size_t header_end = conn->stream_buffer.find("\r\n\r\n");

            if (header_end == std::string::npos) break;

            std::string header_part = conn->stream_buffer.substr(0, header_end + 4);

            HttpRequest temp_req = HttpParser::parse(header_part);

            size_t content_length = 0;
            std::string len_str = temp_req.get_header("Content-Length");

            if (!len_str.empty()) {
                content_length = std::stoul(len_str);
            }

            size_t total_size = header_end + 4 + content_length;

            if (conn->stream_buffer.size() < total_size) break;

            std::string raw_req = conn->stream_buffer.substr(0, total_size);

            conn->stream_buffer.erase(0, total_size);

            HttpRequest req = HttpParser::parse(raw_req);

            HttpResponse res = router.route(req);

            bool keep_alive = (req.get_header("Connection") != "close");

            if (keep_alive) {
                res.set_header("Connection", "keep-alive");
            }
            else {
                res.set_header("Connection", "close");
            }

            std::string response = res.to_string();

            if (!send_all(conn->fd, response)) {
                closed = true;
                break;
            }

            if (!keep_alive) {
                closed = true;
                break;
            }
        }
    }

    if (closed) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->fd, nullptr);
        close(conn->fd);
        delete conn;
    } else {
        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        ev.data.ptr = conn;
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->fd, &ev);
    }
}