#include <set>
#include <string>
#include <fstream>
#include <streambuf>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/thread_pool.hpp"   /* for `run_in_blocker_pool()` */
#include "http/file_app.hpp"
#include "logger.hpp"
#include "stl_utils.hpp"

file_http_app_t::file_http_app_t(std::set<std::string> _whitelist, std::string _asset_dir)
    : whitelist(_whitelist), asset_dir(_asset_dir)
{ }

http_res_t file_http_app_t::handle(const http_req_t &req) {
    if (req.method != GET) {
        /* Method not allowed. */
        return http_res_t(405);
    }
    std::string resource(req.resource.as_string());
    if (resource != "/" && resource != "" && !std_contains(whitelist, resource)) {
        logINF("Someone asked for the nonwhitelisted file %s, if this should be accessible add it to the whitelist.", resource.c_str());
        return http_res_t(403);
    }

    http_res_t res;
    std::string filename;

    if (resource == "/" || resource == "") {
        filename = "/index.html";
    } else {
        filename = resource;
    }

    thread_pool_t::run_in_blocker_pool(boost::bind(&file_http_app_t::handle_blocking, this, filename, &res));

    return res;
}

void file_http_app_t::handle_blocking(std::string filename, http_res_t *res_out) {
    // FIXME: make sure that we won't walk out of our sandbox! Check symbolic links, etc.
    std::ifstream f((asset_dir + filename).c_str());

    if (f.fail()) {
        logINF("File %s was requested and is on the whitelist but we didn't find it in the directory.", (asset_dir + filename).c_str());
        *res_out = http_res_t(404);
        return;
    }

    f.seekg(0, std::ios::end);

    if (f.fail()) {
        goto INTERNAL_ERROR;
    }

    res_out->body.reserve(f.tellg());

    if (f.fail()) {
        goto INTERNAL_ERROR;
    }

    f.seekg(0, std::ios::beg);

    if (f.fail()) {
        goto INTERNAL_ERROR;
    }

    res_out->body.assign((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

    res_out->code = 200;

    return;

INTERNAL_ERROR:
    *res_out = http_res_t(500);
}
