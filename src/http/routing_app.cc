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

http_res_t routing_http_app_t::handle(const http_req_t &req) {
    http_req_t::resource_t::iterator it = req.resource.begin();
    if (it == req.resource.end() || !std_contains(subroutes, *it)) {
        /* if we don't have a route for this, or no route was specified see if the default server knows anything about it */
        if (defaultroute) {
            return defaultroute->handle(req);
        } else {
            return http_res_t(HTTP_NOT_FOUND);
        }
    } else {
        std::string route(*it);
        ++it;
        return subroutes[route]->handle(http_req_t(req, it));
    }
}
