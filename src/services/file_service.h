#ifndef HTTP_SERVER_STARTER_CPP_FILE_SERVER_H
#define HTTP_SERVER_STARTER_CPP_FILE_SERVER_H
#include <string>

class FileService {
    public:
    explicit FileService(const std::string& base_dir);
    bool exists(const std::string& filename) const;
    std::string read(const std::string& filename) const;

    private:
    std::string base_directory;
};
#endif //HTTP_SERVER_STARTER_CPP_FILE_SERVER_H