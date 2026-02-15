#include "router.h"
#include <cstring>
#include <zlib.h>

Router::Router(const std::string &directory) : file_service(directory) {}

std::string gzip_compress(const std::string& data) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    // windowBits = 15 + 16 for gzip encoding
    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return data;
    }

    zs.next_in = (Bytef*)data.data();
    zs.avail_in = data.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);
    return outstring;
}

HttpResponse Router::route(const HttpRequest& req) const {
    if (req.method == "GET") {
        if (req.path == "/") {
            return {200};
        }

        if (req.path.rfind("/echo/", 0) == 0) {
            std::string msg = req.path.substr(6);
            HttpResponse res(200);

            std::string accept_encoding = req.get_header("Accept-Encoding");
            if (accept_encoding.find("gzip") != std::string::npos) {
                res.set_header("Content-Encoding", "gzip");
                res.set_body(gzip_compress(msg));
            }
            else {
                res.set_body(msg);
            }

            res.set_header("Content-Type", "text/plain");

            return res;
        }

        if (req.path == "/user-agent") {
            HttpResponse res(200);
            res.set_header("Content-Type", "text/plain");
            res.set_body(req.get_header("User-Agent"));
            return res;
        }

        if (req.path.rfind("/files/", 0) == 0) {
            std::string filename = req.path.substr(7);

            if (!file_service.exists(filename)) {
                return {404};
            }

            HttpResponse res(200);
            res.set_header("Content-Type", "application/octet-stream");
            res.set_body(file_service.read(filename));
            return res;
        }
    }

    if (req.method == "POST") {
        if (req.path.rfind("/files/", 0) == 0) {
            std::string filename = req.path.substr(7);

            bool success = file_service.write(filename, req.body);
            if (!success) {
                return {400};
            }
            return {201};
        }
    }

    return {404};
}