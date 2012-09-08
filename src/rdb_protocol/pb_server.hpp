#ifndef RDB_PROTOCOL_PB_SERVER_HPP_
#define RDB_PROTOCOL_PB_SERVER_HPP_

#include <map>

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

private:
    Response handle(Query *q, stream_cache_t *stream_cache);

    protob_server_t<Query, Response, stream_cache_t> server;
    rdb_protocol_t::context_t *ctx;
};

Response on_unparsable_query(Query *q, std::string msg);

#endif /* RDB_PROTOCOL_PB_SERVER_HPP_ */
