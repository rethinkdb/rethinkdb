// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/wait_for_readiness.hpp"

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "arch/timing.hpp"
#include "concurrency/signal.hpp"
#include "containers/uuid.hpp"
#include "protocol_api.hpp"
#include "rdb_protocol/batching.hpp"
#include "rdb_protocol/protocol.hpp"

bool test_for_rdb_table_readiness(namespace_interface_t *namespace_if,
                                  signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    // This read won't succeed, but we care whether it fails with an exception.
    // It must be an rget to make sure that access is available to all shards.
    rget_read_t empty_rget_read(
        hash_region_t<key_range_t>::universe(),
        std::map<std::string, ql::wire_func_t>(),
        "",
        ql::batchspec_t::user(ql::batch_type_t::NORMAL, counted_t<const ql::datum_t>()),
        std::vector<ql::transform_variant_t>(),
        boost::optional<ql::terminal_variant_t>(),
        boost::optional<sindex_rangespec_t>(),
        sorting_t::UNORDERED);
    read_t empty_read(empty_rget_read, profile_bool_t::DONT_PROFILE);
    try {
        read_response_t read_res;
        // TODO: We should not use order_token_t::ignore.
        namespace_if->read(empty_read, &read_res, order_token_t::ignore, interruptor);
        return true;
    } catch (const cannot_perform_query_exc_t &e) {
        return false;
    }
}

