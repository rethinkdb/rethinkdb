// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/wait_for_readiness.hpp"

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "arch/timing.hpp"
#include "concurrency/signal.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/batching.hpp"
#include "rdb_protocol/protocol.hpp"

void wait_for_rdb_table_readiness(base_namespace_repo_t *ns_repo,
                                  uuid_u namespace_id,
                                  signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    /* The following is an ugly hack, but it's probably what we want.  It
       takes about a third of a second for the new namespace to get to the
       point where we can do reads/writes on it.  We don't want to return
       until that has happened, so we try to do a read every `poll_ms`
       milliseconds until one succeeds, then return. */

    // This read won't succeed, but we care whether it fails with an exception.
    // It must be an rget to make sure that access is available to all shards.

    const int poll_ms = 20;
    const int deleted_check_ms = 4000;
    const int deleted_check_interval = std::max(deleted_check_ms / poll_ms, 1);
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
    for (int num_attempts = 0; true; ++num_attempts) {
        nap(poll_ms, interruptor);
        try {
            // Make sure the namespace still exists in the metadata, if not, abort.
            // ... However don't to it too often. If we have a large cluster,
            // copying the semilattice_metadata becomes quite expensive. So
            // we want to avoid that as much as possible.
            if ((num_attempts + 1) % deleted_check_interval == 0) {
                bool exists = ns_repo->check_namespace_exists(namespace_id, interruptor);
                /* Throwing `interrupted_exc_t()` doesn't quite make sense here. */
                if (!exists) throw interrupted_exc_t();
            }

            base_namespace_repo_t::access_t ns_access(
                ns_repo, namespace_id, interruptor);
            read_response_t read_res;
            // TODO: We should not use order_token_t::ignore.
            ns_access.get_namespace_if()->read(
                empty_read, &read_res, order_token_t::ignore, interruptor);
            break;
        } catch (const cannot_perform_query_exc_t &e) {
            // continue loop
        }
    }
}

