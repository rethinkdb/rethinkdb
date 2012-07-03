#ifndef RDB_PROTOCOL_PARSER_HPP_
#define RDB_PROTOCOL_PARSER_HPP_

#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "http/http.hpp"
#include "rdb_protocol/protocol.hpp"
#include "protocol_api.hpp"
#include "rpc/mailbox/mailbox.hpp"

namespace rdb_protocol {

class query_http_app_t : public http_app_t {
public:
    explicit query_http_app_t(namespace_interface_t<rdb_protocol_t> * _namespace_if);
    http_res_t handle(const http_req_t &);

private:
    namespace_interface_t<rdb_protocol_t> *namespace_if;
    order_source_t order_source;
};

class protocol_parser_t {
public:
    protocol_parser_t(int port, namespace_interface_t<rdb_protocol_t> *nif, perfmon_collection_t *) :
        query_app(nif),
        server(port, &query_app)
        { }

private:
    query_http_app_t query_app;
    http_server_t server;
};

} //namespace rdb_protocol

#endif /* RDB_PROTOCOL_PARSER_HPP_ */
