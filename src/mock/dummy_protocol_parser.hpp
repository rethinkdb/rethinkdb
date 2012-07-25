#ifndef MOCK_DUMMY_PROTOCOL_PARSER_HPP_
#define MOCK_DUMMY_PROTOCOL_PARSER_HPP_

#include "errors.hpp"
#include <boost/ptr_container/ptr_map.hpp>

#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "http/http.hpp"
#include "mock/dummy_protocol.hpp"
#include "protocol_api.hpp"
#include "rpc/mailbox/mailbox.hpp"
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
    dummy_protocol_parser_t(int port, namespace_interface_t<dummy_protocol_t> *nif, perfmon_collection_t *) :
        query_app(nif),
        server(port, &query_app)
        { }

private:
    query_http_app_t query_app;
    http_server_t server;
};

} //namespace mock

#endif /* MOCK_DUMMY_PROTOCOL_PARSER_HPP_ */
