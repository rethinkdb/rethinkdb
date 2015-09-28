// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QUERY_CACHE_HPP_
#define RDB_PROTOCOL_QUERY_CACHE_HPP_

#include <time.h>

#include <exception>
#include <map>
#include <set>
#include <string>

#include "arch/address.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/new_mutex.hpp"
#include "concurrency/wait_any.hpp"
#include "concurrency/signal.hpp"
#include "concurrency/watchable.hpp"
#include "containers/scoped.hpp"
#include "containers/counted.hpp"
#include "containers/intrusive_list.hpp"
#include "containers/object_buffer.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/backtrace.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/query_params.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/term_storage.hpp"
#include "rdb_protocol/wire_func.hpp"

namespace ql {

class query_cache_t : public home_thread_mixin_t {
    class entry_t;
public:
    query_cache_t(rdb_context_t *_rdb_ctx,
                  ip_and_port_t _client_addr_port,
                  return_empty_normal_batches_t _return_empty_normal_batches);
    ~query_cache_t();

    // A reference to a given query in the cache - no more than one reference may be
    //  held for a given query at any time.
    class ref_t {
    public:
        ~ref_t();
        void fill_response(response_t *res);
    private:
        friend class query_cache_t;
        ref_t(query_cache_t *_query_cache,
              int64_t _token,
              new_semaphore_acq_t _throttler,
              query_cache_t::entry_t *_entry,
              signal_t *interruptor);

        // Run a new query
        void run(env_t *env, response_t *res);
        // Serve a batch from a stream
        void serve(env_t *env, response_t *res);

        query_cache_t::entry_t *const entry;
        const int64_t token;
        const scoped_ptr_t<profile::trace_t> trace;

        query_cache_t *query_cache;
        new_semaphore_acq_t throttler;
        auto_drainer_t::lock_t drainer_lock;
        wait_any_t combined_interruptor;
        new_mutex_in_line_t mutex_lock;

        DISABLE_COPYING(ref_t);
    };

    // const iteration for the jobs table
    typedef std::map<int64_t, scoped_ptr_t<entry_t> >::const_iterator const_iterator;
    const_iterator begin() const;
    const_iterator end() const;

    // Helper function used by the jobs table
    ip_and_port_t get_client_addr_port() const { return client_addr_port; }

    // Methods to obtain a unique reference to a given entry in the cache
    scoped_ptr_t<ref_t> create(query_params_t *query_params,
                               signal_t *interruptor);

    scoped_ptr_t<ref_t> get(query_params_t *query_params,
                            signal_t *interruptor);

    void noreply_wait(const query_params_t &query_params,
                      signal_t *interruptor);

    // Interrupt a query by token
    void terminate_query(const query_params_t &query_params);

private:
    class entry_t {
    public:
        entry_t(query_params_t *query_params,
                global_optargs_t &&_global_optargs,
                counted_t<const term_t> &&_term_tree);
        ~entry_t();

        enum class state_t { START, STREAM, DONE, DELETING } state;

        const uuid_u job_id;
        const bool noreply;
        const profile_bool_t profile;
        const scoped_ptr_t<const term_storage_t> term_storage;
        const global_optargs_t global_optargs;
        const microtime_t start_time;

        cond_t persistent_interruptor;

        // This will be empty if the root term has already been run
        counted_t<const term_t> term_tree;

        // This will be empty until the root term has been evaluated
        // If this resulted in a stream, this will not be empty until the
        // stream is finished
        counted_t<datum_stream_t> stream;
        bool has_sent_batch;

        // The order of these is very important, do not move them around
        new_mutex_t mutex; // Only one coroutine may be using this query at a time
        auto_drainer_t drainer; // Keep this entry alive until all refs are destroyed

    private:
        DISABLE_COPYING(entry_t);
    };

    void terminate_internal(entry_t *entry);
    static void async_destroy_entry(entry_t *entry);

    rdb_context_t *const rdb_ctx;
    ip_and_port_t client_addr_port;
    return_empty_normal_batches_t return_empty_normal_batches;
    std::map<int64_t, scoped_ptr_t<entry_t> > queries;

    // Used for noreply waiting, this contains all allocated-but-incomplete query ids
    friend class query_params_t::query_id_t;
    uint64_t next_query_id;
    intrusive_list_t<query_params_t::query_id_t> outstanding_query_ids;
    watchable_variable_t<uint64_t> oldest_outstanding_query_id;

    DISABLE_COPYING(query_cache_t);
};

} // namespace ql

#endif  // RDB_PROTOCOL_QUERY_CACHE_HPP_
