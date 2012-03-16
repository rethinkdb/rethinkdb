#include "errors.hpp"
#include <boost/tokenizer.hpp>

#include "http/routing_app.hpp"
#include "stl_utils.hpp"

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
typedef tokenizer::iterator tok_iterator;

routing_http_app_t::routing_http_app_t(http_app_t *_defaultroute, std::map<std::string, http_app_t *> _subroutes)
    : subroutes(_subroutes), defaultroute(_defaultroute)
{ }

/* Routes should not have any slashes in them */
void sanitize_routes(DEBUG_ONLY_VAR const std::map<std::string, http_app_t *> routes) {
#ifndef NDEBUG
    for (std::map<std::string, http_app_t *>::const_iterator it =  routes.begin();
                                                                it != routes.end();
                                                                it++) {
        rassert(strchr(it->first.c_str(), '/') == NULL, "Routes should not contain '/'s");
    }
#endif
}

void routing_http_app_t::add_route(const std::string& route, http_app_t *server) {
    subroutes[route] = server;
    sanitize_routes(subroutes);
}

http_res_t routing_http_app_t::handle(const http_req_t &req) {
    //setup a tokenizer
    boost::char_separator<char> sep("/");
    tokenizer tokens(req.resource, sep);
    tok_iterator it = tokens.begin();

    if (it == tokens.end() || !std_contains(subroutes, *it)) {
        /* if we don't have a route for this, or no route was specified see if the default server knows anything about it */
        if (defaultroute) {
            return defaultroute->handle(req);
        } else {
            return http_res_t(404);
        }
    } else {
        http_req_t copy = req;

        std::string route = *it;
        it++;

        copy.resource = "";

        // FIXME: this is not very efficient. Split the request into parts once and then only work with parts in the rest of the code.
        while(it != tokens.end()) {
            copy.resource += "/";
            copy.resource += *it;
            it++;
        }

        return subroutes[route]->handle(copy);
    }
}
