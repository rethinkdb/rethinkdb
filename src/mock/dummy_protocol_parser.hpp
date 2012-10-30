// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MOCK_DUMMY_PROTOCOL_PARSER_HPP_
#define MOCK_DUMMY_PROTOCOL_PARSER_HPP_

#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "http/http.hpp"
#include "mock/dummy_protocol.hpp"
#include "protocol_api.hpp"
#include "rpc/semilattice/view.hpp"

namespace mock {

class query_http_app_t : public http_app_t {
public:
    explicit query_http_app_t(namespace_interface_t<dummy_protocol_t> * _namespace_if);
    http_res_t handle(const http_req_t &);

private:
    namespace_interface_t<dummy_protocol_t> *namespace_if;
    order_source_t order_source;
};

class dummy_protocol_parser_t {
public:
    dummy_protocol_parser_t(int port, namespace_repo_t<dummy_protocol_t> *ns_repo, const namespace_id_t &ns_id, perfmon_collection_t *) :
        ns_access(ns_repo, ns_id, &interruptor),
        query_app(ns_access.get_namespace_if()),
        server(port, &query_app)
        { }

    signal_t *get_bound_signal() {
        return server.get_bound_signal();
    }

private:
    cond_t interruptor; // Not used, but needed to construct the ns_access
    namespace_repo_t<dummy_protocol_t>::access_t ns_access;
    query_http_app_t query_app;
    http_server_t server;
};

} //namespace mock

#endif /* MOCK_DUMMY_PROTOCOL_PARSER_HPP_ */
