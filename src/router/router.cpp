#include "router.h"

Router::Router(const std::string &directory) : file_service(directory) {}

HttpResponse Router::route(const HttpRequest& req) {
    if (req.path == "/") {
        return HttpResponse(200);
    }

    if (req.path.rfind("/echo/", 0) == 0) {
        std::string msg = req.path.substr(6);
        HttpResponse res(200);
        res.set_header("Content-Type", "text/plain");
        res.set_body(msg);
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
            return HttpResponse(404);
        }

        HttpResponse res(200);
        res.set_header("Content-Type",
                       "application/octet-stream");
        res.set_body(file_service.read(filename));
        return res;
    }

    return HttpResponse(404);
}
