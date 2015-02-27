// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "http/routing_app.hpp"

#include <string.h>

#include "stl_utils.hpp"

routing_http_app_t::routing_http_app_t(http_app_t *_defaultroute, std::map<std::string, http_app_t *> _subroutes)
    : subroutes(_subroutes), defaultroute(_defaultroute)
{ }

void routing_http_app_t::add_route(const std::string& route, http_app_t *server) {
    guarantee(strchr(route.c_str(), '/') == NULL, "Routes should not contain '/'s");
    subroutes[route] = server;
}

void routing_http_app_t::handle(const http_req_t &req, http_res_t *result, signal_t *interruptor) {
    http_req_t::resource_t::iterator it = req.resource.begin();
    if (it == req.resource.end() || !std_contains(subroutes, *it)) {
        // if we don't have a route for this, or no route was specified, let the default server handle it
        if (defaultroute) {
            defaultroute->handle(req, result, interruptor);
        } else {
            *result = http_res_t(http_status_code_t::NOT_FOUND);
        }
    } else {
        std::string route(*it);
        ++it;
        subroutes[route]->handle(http_req_t(req, it), result, interruptor);
    }
}
