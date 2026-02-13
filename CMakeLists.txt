cmake_minimum_required(VERSION 3.13)

project(http-server-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(http-server ${SOURCE_FILES}
        src/server/http_server.h
        src/server/http_server.cpp
        src/http/http_request.h
        src/http/http_response.h
        src/http/http_response.cpp
        src/http/http_parser.h
        src/http/http_parser.cpp
        src/services/file_service.h
        src/services/file_service.cpp
        src/router/router.h
        src/router/router.cpp)

target_link_libraries(http-server PRIVATE Threads::Threads ZLIB::ZLIB)
