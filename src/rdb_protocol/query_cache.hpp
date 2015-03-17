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
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/ql2.pb.h"

namespace ql {
class env_t;
}

namespace ql {

class query_cache_exc_t : public std::exception {
public:
    query_cache_exc_t(Response_ResponseType _type,
                      std::string _message,
                      backtrace_t _bt);
    ~query_cache_exc_t() throw ();

    void fill_response(Response *res) const;
    const char *what() const throw ();

    const Response_ResponseType type;
    const std::string message;
    const backtrace_t bt;
};

// A query id, should be allocated when each query is received from the client
// in order, so order can be checked for in queries that require it
class query_id_t : public intrusive_list_node_t<query_id_t> {
public:
    explicit query_id_t(query_cache_t *parent);
    query_id_t(query_id_t &&other);
    ~query_id_t();

    uint64_t value() const;

private:
    query_cache_t *parent;
    uint64_t value_;
    DISABLE_COPYING(query_id_t);
};

class query_cache_t : public home_thread_mixin_t {
    struct entry_t;
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

        void fill_response(Response *res);
        void terminate();
    private:
        friend class query_cache_t;
        ref_t(query_cache_t *_query_cache,
              int64_t _token,
              query_cache_t::entry_t *_entry,
              use_json_t _use_json,
              signal_t *interruptor);

        void run(env_t *env, Response *res); // Run a new query
        void serve(env_t *env, Response *res); // Serve a batch from a stream

        query_cache_t::entry_t *const entry;
        const int64_t token;
        const scoped_ptr_t<profile::trace_t> trace;
        const use_json_t use_json;

        query_cache_t *query_cache;
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
    scoped_ptr_t<ref_t> create(int64_t token,
                               protob_t<Query> original_query,
                               use_json_t use_json,
                               signal_t *interruptor);

    scoped_ptr_t<ref_t> get(int64_t token,
                            use_json_t use_json,
                            signal_t *interruptor);

    void noreply_wait(const query_id_t &query_id,
                      int64_t token,
                      signal_t *interruptor);

private:
    struct entry_t {
        entry_t(protob_t<Query> original_query,
                std::map<std::string, wire_func_t> &&global_optargs,
                counted_t<const term_t> root_term);
        ~entry_t();

        enum class state_t { START, STREAM, DONE, DELETING } state;

        const uuid_u job_id;
        const protob_t<Query> original_query;
        const std::map<std::string, wire_func_t> global_optargs;
        const profile_bool_t profile;
        const microtime_t start_time;

        cond_t persistent_interruptor;

        // This will be empty if the root term has already been run
        counted_t<const term_t> root_term;

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

    static void async_destroy_entry(entry_t *entry);

    rdb_context_t *const rdb_ctx;
    ip_and_port_t client_addr_port;
    return_empty_normal_batches_t return_empty_normal_batches;
    std::map<int64_t, scoped_ptr_t<entry_t> > queries;

    // Used for noreply waiting, this contains all allocated-but-incomplete query ids
    friend class query_id_t;
    uint64_t next_query_id;
    intrusive_list_t<query_id_t> outstanding_query_ids;
    watchable_variable_t<uint64_t> oldest_outstanding_query_id;

    DISABLE_COPYING(query_cache_t);
};

} // namespace ql

#endif  // RDB_PROTOCOL_QUERY_CACHE_HPP_
