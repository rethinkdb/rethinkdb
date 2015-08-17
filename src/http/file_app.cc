// Copyright 2010-2012 RethinkDB, all rights reserved.

#include <time.h>

#include <string>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "debug.hpp"
#include "arch/runtime/thread_pool.hpp"   /* for `run_in_blocker_pool()` */
#include "containers/archive/file_stream.hpp"
#include "http/file_app.hpp"
#include "logger.hpp"
#include "stl_utils.hpp"
#include "time.hpp"
#include "http/web_assets.hpp"

file_http_app_t::file_http_app_t(std::string _asset_dir)
    : asset_dir(_asset_dir)
{ }

bool ends_with(const std::string& str, const std::string& end) {
    return str.rfind(end) == str.length() - end.length();
}

void file_http_app_t::handle(const http_req_t &req, http_res_t *result, signal_t *) {
    if (req.method != http_method_t::GET) {
        *result = http_res_t(http_status_code_t::METHOD_NOT_ALLOWED);
        return;
    }

    std::string resource(req.resource.as_string());

    std::string filename;

    if (resource == "/" || resource == "") {
        filename = "/index.html";
    } else {
        filename = resource;
    }

    auto it = static_web_assets.find(filename);
    if (it == static_web_assets.end()) {
        printf_buffer_t resource_buffer;
        debug_print_quoted_string(&resource_buffer,
                                  reinterpret_cast<const uint8_t *>(resource.data()),
                                  resource.length());
        logNTC("Someone asked for the nonwhitelisted file %s.  If this should be "
               "accessible, add it to the static web assets.", resource_buffer.c_str());
        *result = http_res_t(http_status_code_t::FORBIDDEN);
        return;
    }

    const std::string &resource_data = it->second;

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
    result->add_header_line("Content-Type", mimetype);

    if (asset_dir.empty()) {
        result->body.assign(resource_data.begin(), resource_data.end());
        result->code = http_status_code_t::OK;
    } else {
        thread_pool_t::run_in_blocker_pool(boost::bind(&file_http_app_t::handle_blocking, this, filename, result));

        if (result->code == http_status_code_t::NOT_FOUND) {
            logNTC("File %s was requested and is on the whitelist but we didn't find it in the directory.", (asset_dir + filename).c_str());
        }
    }
}

void file_http_app_t::handle_blocking(std::string filename, http_res_t *res_out) {

    // FIXME: make sure that we won't walk out of our sandbox! Check symbolic links, etc.
    blocking_read_file_stream_t stream;
    bool initialized = stream.init((asset_dir + filename).c_str());

    if (!initialized) {
        res_out->code = http_status_code_t::NOT_FOUND;
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
            res_out->code = http_status_code_t::INTERNAL_SERVER_ERROR;
            return;
        }
    }

    res_out->body.assign(body.begin(), body.end());
    res_out->code = http_status_code_t::OK;
}
