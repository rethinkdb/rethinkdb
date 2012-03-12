#include <set>
#include <string>
#include <fstream>
#include <streambuf>

#include "http/file_server.hpp"
#include "logger.hpp"
#include "stl_utils.hpp"

http_file_server_t::http_file_server_t(int port, std::set<std::string> _whitelist, std::string _asset_dir) 
    : http_server_t(port), whitelist(_whitelist), asset_dir(_asset_dir)
{ }

http_file_server_t::http_file_server_t(std::set<std::string> _whitelist, std::string _asset_dir) 
    : whitelist(_whitelist), asset_dir(_asset_dir)
{ }

http_res_t http_file_server_t::handle(const http_req_t &req) {
    if (!std_contains(whitelist, req.resource)) {
        logINF("Someone asked for the nonwhitelisted file %s, if this should be accessible add it to the whitelist.\n", req.resource.c_str());
        return http_res_t(403);
    }

    http_res_t res;
    std::ifstream f((asset_dir + req.resource).c_str());
    
    if (f.fail()) {
        logINF("File %s was requested and is on the whitelist but we didn't find it in the directory.\n", (asset_dir + req.resource).c_str());
        return http_res_t(404);
    }

    f.seekg(0, std::ios::end);   
    
    if (f.fail()) {
        goto INTERNAL_ERROR;
    }

    res.body.reserve(f.tellg());

    if (f.fail()) {
        goto INTERNAL_ERROR;
    }

    f.seekg(0, std::ios::beg);

    if (f.fail()) {
        goto INTERNAL_ERROR;
    }

    res.body.assign((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());

    res.code = 200;

    return res;

INTERNAL_ERROR:
    return http_res_t(500);
}
