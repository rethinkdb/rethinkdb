#include <set>
#include <string>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/thread_pool.hpp"   /* for `run_in_blocker_pool()` */
#include "containers/archive/file_stream.hpp"
#include "http/file_app.hpp"
#include "logger.hpp"
#include "stl_utils.hpp"

file_http_app_t::file_http_app_t(std::set<std::string> _whitelist, std::string _asset_dir)
    : whitelist(_whitelist), asset_dir(_asset_dir)
{ }

http_res_t file_http_app_t::handle(const http_req_t &req) {
    if (req.method != GET) {
        /* Method not allowed. */
        return http_res_t(HTTP_METHOD_NOT_ALLOWED);
    }

    std::string resource(req.resource.as_string());
#ifndef NDEBUG
    if (resource != "/" && resource != "" && !std_contains(whitelist, resource)) {
        logINF("Someone asked for the nonwhitelisted file %s, if this should be accessible add it to the whitelist.", resource.c_str());
        return http_res_t(HTTP_FORBIDDEN);
    }
#endif

    http_res_t res;
    std::string filename;

    if (resource == "/" || resource == "") {
        filename = "/index.html";
    } else {
        filename = resource;
    }

    thread_pool_t::run_in_blocker_pool(boost::bind(&file_http_app_t::handle_blocking, this, filename, &res));

    if (res.code == 404) {
        logINF("File %s was requested and is on the whitelist but we didn't find it in the directory.", (asset_dir + filename).c_str());
    }

    return res;
}

bool ends_with(const std::string& str, const std::string& end) {
    return str.rfind(end) == str.length() - end.length();
}

void file_http_app_t::handle_blocking(std::string filename, http_res_t *res_out) {

    // TODO more robust mimetype detection?
    std::string mimetype = "text/plain";
    if (ends_with(filename, ".js")) {
        mimetype = "text/javascript";
    } else if (ends_with(filename, ".css")) {
        mimetype = "text/css";
    } else if (ends_with(filename, ".html")) {
        mimetype = "text/html";
    }

    // FIXME: make sure that we won't walk out of our sandbox! Check symbolic links, etc.
    blocking_read_file_stream_t stream;
    bool initialized = stream.init((asset_dir + filename).c_str());

    res_out->add_header_line("Content-Type", mimetype);

    if (!initialized) {
        res_out->code = 404;
        return;
    }

    std::vector<char> body;

    for (;;) {
        const int bufsize = 4096;
        char buf[bufsize];
        int64_t res = stream.read(buf, bufsize);

        if (res > 0) {
            body.insert(body.end(), buf, buf + res);
        } else if (res == 0) {
            break;
        } else {
            res_out->code = 500;
            return;
        }
    }

    res_out->body.assign(body.begin(), body.end());
    res_out->code = 200;
}
