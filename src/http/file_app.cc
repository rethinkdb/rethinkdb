// Copyright 2010-2012 RethinkDB, all rights reserved.

#include <time.h>

#include <set>
#include <string>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/thread_pool.hpp"   /* for `run_in_blocker_pool()` */
#include "containers/archive/file_stream.hpp"
#include "http/file_app.hpp"
#include "logger.hpp"
#include "stl_utils.hpp"
#include "time.hpp"

file_http_app_t::file_http_app_t(std::set<std::string> _whitelist, std::string _asset_dir)
    : whitelist(_whitelist), asset_dir(_asset_dir)
{ }

void file_http_app_t::handle(const http_req_t &req, http_res_t *result, signal_t *) {
    if (req.method != GET) {
        *result = http_res_t(HTTP_METHOD_NOT_ALLOWED);
        return;
    }

    std::string resource(req.resource.as_string());
    if (resource != "/" && resource != "" && !std_contains(whitelist, resource)) {
        logINF("Someone asked for the nonwhitelisted file %s, if this should be accessible add it to the whitelist.", resource.c_str());
        *result = http_res_t(HTTP_FORBIDDEN);
        return;
    }

    std::string filename;

    if (resource == "/" || resource == "") {
        filename = "/index.html";
    } else {
        filename = resource;
    }

    time_t expires;
#ifndef NDEBUG
    // In debug mode, do not cache static web assets
    expires = get_secs() - 31536000; // Some time in the past (one year ago)
#else
    // In release mode, cache static web assets except index.html
    if (filename == "/index.html") {
        expires = get_secs() - 31536000; // Some time in the past (one year ago)
    } else {
        expires = get_secs() + 31536000; // One year from now
    }
#endif
    result->add_header_line("Expires", http_format_date(expires));

    thread_pool_t::run_in_blocker_pool(boost::bind(&file_http_app_t::handle_blocking, this, filename, result));

    if (result->code == 404) {
        logINF("File %s was requested and is on the whitelist but we didn't find it in the directory.", (asset_dir + filename).c_str());
    }
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
    } else if (ends_with(filename, ".woff")) {
        mimetype = "application/font-woff";
    } else if (ends_with(filename, ".eot")) {
        mimetype = "application/vnd.ms-fontobject";
    } else if (ends_with(filename, ".ttf")) {
        mimetype = "application/x-font-ttf";
    } else if (ends_with(filename, ".svg")) {
        mimetype = "image/svg+xml";
    } else if (ends_with(filename, ".ico")) {
        mimetype = "image/vnd.microsoft.icon";
    } else if (ends_with(filename, ".png")) {
        mimetype = "image/png";
    } else if (ends_with(filename, ".gif")) {
        mimetype = "image/gif";
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
