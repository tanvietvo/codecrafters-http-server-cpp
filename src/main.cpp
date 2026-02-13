#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <thread>

void handle_client(int client_fd, const std::string &directory) {
  char buffer[1024] = {0};
  int bytes_read = recv(client_fd, buffer, 1024, 0);

  if (bytes_read > 0) {
    std::string request = std::string(buffer);

    size_t first_space = request.find(" ");
    size_t second_space = request.find(" ", first_space + 1);

    if (first_space != std::string::npos && second_space != std::string::npos) {
      std::string path = request.substr(first_space + 1, second_space - first_space - 1);

      std::string response = "HTTP/1.1 ";
      if (path == "/")
        response += "200 OK\r\n"
                    "\r\n";
      else if (path.find("/echo/") == 0) {
        std::string extracted_path = path.substr(6);
        response += "200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: " + std::to_string(extracted_path.length()) + "\r\n"
                    "\r\n" +
                    extracted_path;
      }
      else if (path == "/user-agent") {
        std::string term = "User-Agent: ";
        size_t term_pos = request.find(term);

        if (term_pos != std::string::npos) {
          size_t start_pos = term_pos + term.length();
          size_t end_pos = request.find("\r\n", start_pos);

          std::string value = request.substr(start_pos, end_pos - start_pos);

          response += "200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: " + std::to_string(value.length()) + "\r\n"
                    "\r\n" +
                    value;
        }
      }
      else if (path.find("/files/") == 0) {
        std::string file_name = path.substr(7);
        std::string file_path = directory + file_name;

        std::ifstream file(file_path, std::ios::binary);
        if (file.is_open()) {
          std::stringstream buffer;
          buffer << file.rdbuf();
          std::string content = buffer.str();

          response += "200 OK\r\n"
                     "Content-Type: application/octet-stream\r\n"
                     "Content-Length: " + std::to_string(content.length()) + "\r\n"
                     "\r\n" +
                     content;
        }
      }
      else
        response += "404 Not Found\r\n"
                    "\r\n";

      send(client_fd, response.c_str(), response.length(), 0);
      close(client_fd);
    }
  }
}

int main(int argc, char **argv) {
  std::string directory = "";
  if (argc == 3 && std::string(argv[1]) == "--directory") {
    directory = std::string(argv[2]);
  }
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // TODO: Uncomment the code below to pass the first stage
  //
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  while (true) {
    int client_socket = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t*)& client_addr_len);
    if (client_socket < 0) {
      std::cerr << "accept failed\n";
      continue;
    }


      std::thread(handle_client, client_socket, directory).detach();
  }

  close(server_fd);
  return 0;
}
