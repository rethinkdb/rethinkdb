// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef HTTP_ROUTING_APP_HPP_
#define HTTP_ROUTING_APP_HPP_

#include <map>
#include <string>

#include "http/http.hpp"

class routing_http_app_t : public http_app_t {
public:
    routing_http_app_t(http_app_t *defaultroute, std::map<std::string, http_app_t *> subroutes);

    void add_route(const std::string& route, http_app_t *server);

    void handle(const http_req_t &, http_res_t *result, signal_t *interruptor);

private:
    std::map<std::string, http_app_t *> subroutes;
    http_app_t *defaultroute;
};

#endif /* HTTP_ROUTING_APP_HPP_ */
