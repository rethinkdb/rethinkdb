// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/query_language.hpp"

#include <cmath>

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/variant.hpp>

#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/suggester.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/stream_cache.hpp"
#include "rdb_protocol/proto_utils.hpp"
#include "rpc/directory/read_manager.hpp"


//TODO: why is this not in the query_language namespace? - because it's also used by rethinkdb import at the moment
void wait_for_rdb_table_readiness(namespace_repo_t<rdb_protocol_t> *ns_repo,
                                  namespace_id_t namespace_id,
                                  signal_t *interruptor,
                                  boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata) THROWS_ONLY(interrupted_exc_t) {
    /* The following is an ugly hack, but it's probably what we want.  It
       takes about a third of a second for the new namespace to get to the
       point where we can do reads/writes on it.  We don't want to return
       until that has happened, so we try to do a read every `poll_ms`
       milliseconds until one succeeds, then return. */

    // This read won't succeed, but we care whether it fails with an exception.
    // It must be an rget to make sure that access is available to all shards.

    const int poll_ms = 10;
    rdb_protocol_t::rget_read_t empty_rget_read(hash_region_t<key_range_t>::universe());
    rdb_protocol_t::read_t empty_read(empty_rget_read);
    for (;;) {
        signal_timer_t start_poll(poll_ms);
        wait_interruptible(&start_poll, interruptor);
        try {
            // Make sure the namespace still exists in the metadata, if not, abort
            {
                // TODO: use a cross thread watchable instead?  not exactly pressed for time here...
                on_thread_t rethread(semilattice_metadata->home_thread());
                cluster_semilattice_metadata_t metadata = semilattice_metadata->get();
                cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&metadata.rdb_namespaces);
                std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<rdb_protocol_t> > >::iterator
                    nsi = change.get()->namespaces.find(namespace_id);
                rassert(nsi != change.get()->namespaces.end());
                if (nsi->second.is_deleted()) throw interrupted_exc_t();
            }

            namespace_repo_t<rdb_protocol_t>::access_t ns_access(ns_repo, namespace_id, interruptor);
            rdb_protocol_t::read_response_t read_res;
            // TODO: We should not use order_token_t::ignore.
            ns_access.get_namespace_if()->read(empty_read, &read_res, order_token_t::ignore, interruptor);
            break;
        } catch (cannot_perform_query_exc_t e) { } //continue loop
    }
}
