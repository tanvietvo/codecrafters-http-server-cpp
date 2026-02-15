#include <fstream>
#include <sstream>
#include "file_service.h"

FileService::FileService(const std::string& dir) : base_directory(dir) {}

bool FileService::exists(const std::string& filename) const {
    std::ifstream file(base_directory + filename);
    return file.good();
}

std::string FileService::read(const std::string& filename) const {
    std::ifstream file(base_directory + filename, std::ios::binary);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool FileService::write(const std::string& filename, const std::string& content) const {
    std::ofstream file(base_directory + filename, std::ios::binary);

    if (!file) {
        return false;
    }

    file << content;
    return true;
}