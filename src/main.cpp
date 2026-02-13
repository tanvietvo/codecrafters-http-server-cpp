#include <iostream>
#include "server/http_server.h"

int main(int argc, char **argv) {
  std::string directory;
  if (argc == 3 && std::string(argv[1]) == "--directory") {
    directory = std::string(argv[2]);
  }
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  HttpServer server(4221, directory);
  server.start();

  return 0;
}
