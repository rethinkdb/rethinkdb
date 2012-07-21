#ifndef HTTP_FILE_APP_HPP_
#define HTTP_FILE_APP_HPP_

#include <string>
#include <set>

#include "http/http.hpp"

class file_http_app_t : public http_app_t {
public:
    file_http_app_t(std::set<std::string> _whitelist, std::string _asset_dir);

    http_res_t handle(const http_req_t &);
private:
    void handle_blocking(std::string filename, http_res_t *res_out);

    std::set<std::string> whitelist;
    std::string asset_dir;
};

#endif /* HTTP_FILE_APP_HPP_ */
