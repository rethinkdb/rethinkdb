// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef HTTP_FILE_APP_HPP_
#define HTTP_FILE_APP_HPP_

#include <string>

#include "http/http.hpp"

class file_http_app_t : public http_app_t {
public:
    explicit file_http_app_t(std::string _asset_dir);

    void handle(const http_req_t &, http_res_t *result, signal_t *interruptor);
private:
    void handle_blocking(std::string filename, http_res_t *res_out);

    std::string asset_dir;
};

#endif /* HTTP_FILE_APP_HPP_ */
