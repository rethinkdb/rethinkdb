#ifndef __HTTP_ROUTING_SERVER_HPP__
#define __HTTP_ROUTING_SERVER_HPP__

#include <map>
#include <string>

#include "http/http.hpp"

class routing_server_t : public http_server_t {
public:
    routing_server_t(int port, http_server_t *defaultroute, std::map<std::string, http_server_t *> subroutes);

    routing_server_t(http_server_t *defaultroute, std::map<std::string, http_server_t *> subroutes);

    void add_route(const std::string& route, http_server_t *server);

    http_res_t handle(const http_req_t &);

private:
    std::map<std::string, http_server_t *> subroutes;
    http_server_t *defaultroute;
};

#endif
