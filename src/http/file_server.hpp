#ifndef __HTTP_FILE_SERVER_HPP__
#define __HTTP_FILE_SERVER_HPP__

#include "http/http.hpp"

class http_file_server_t : public http_server_t {
public:
    http_file_server_t(int port, std::set<std::string> _whitelist, std::string _asset_dir);

    http_res_t handle(const http_req_t &);
private:
    std::set<std::string> whitelist;
    std::string asset_dir;
};

#endif
