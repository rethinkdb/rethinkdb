#ifndef RDB_PROTOCOL_PB_SERVER_HPP_
#define RDB_PROTOCOL_PB_SERVER_HPP_

#include <map>
#include <string>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "extproc/pool.hpp"
#include "protob/protob.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.hpp"

class query_server_t {
public:
    query_server_t(int port, rdb_protocol_t::context_t *_ctx);

    http_app_t *get_http_app();

    struct context_t {
        context_t() : interruptor(0) { }
        stream_cache_t stream_cache;
        signal_t *interruptor;
    };
private:
    Response handle(Query *q, context_t *query_context);
    protob_server_t<Query, Response, context_t> server;
    rdb_protocol_t::context_t *ctx;
};

Response on_unparsable_query(Query *q, std::string msg);

#endif /* RDB_PROTOCOL_PB_SERVER_HPP_ */
