#ifndef __HTTP_FILE_APP_HPP__
#define __HTTP_FILE_APP_HPP__

#include "http/http.hpp"

class file_http_app_t : public http_app_t {
public:
    file_http_app_t(std::set<std::string> _whitelist, std::string _asset_dir);

    http_res_t handle(const http_req_t &);
private:
    std::set<std::string> whitelist;
    std::string asset_dir;
};

#endif
