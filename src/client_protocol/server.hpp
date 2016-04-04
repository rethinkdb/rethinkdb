// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLIENT_PROTOCOL_SERVER_HPP_
#define CLIENT_PROTOCOL_SERVER_HPP_

#include <set>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include "arch/address.hpp"
#include "arch/io/openssl.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/timing.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "containers/archive/archive.hpp"
#include "containers/counted.hpp"
#include "http/http.hpp"
#include "perfmon/perfmon.hpp"
#include "utils.hpp"

class auth_key_t;

class rdb_context_t;
namespace ql {
class query_params_t;
class query_cache_t;
class response_t;
}

class http_conn_cache_t : public repeating_timer_callback_t,
                          public home_thread_mixin_t {
public:
    typedef std::string conn_key_t;

    class http_conn_t : public single_threaded_countable_t<http_conn_t> {
    public:
        http_conn_t(rdb_context_t *rdb_ctx,
                    ip_and_port_t client_addr_port);

        ql::query_cache_t *get_query_cache();
        signal_t *get_interruptor();

        void pulse();
        time_t last_accessed_time() const;

    private:
        cond_t interruptor;
        time_t last_accessed;
        scoped_ptr_t<ql::query_cache_t> query_cache;
        scoped_perfmon_counter_t counter;
        DISABLE_COPYING(http_conn_t);
    };

    // WARNING: http_conn_cache_t might block since it needs to seed a random
    //  number generator. Don't call this except at server startup.
    explicit http_conn_cache_t(uint32_t _http_timeout_sec);
    ~http_conn_cache_t();

    counted_t<http_conn_t> find(const conn_key_t &key);
    conn_key_t create(rdb_context_t *rdb_ctx, ip_and_port_t client_addr_port);
    void erase(const conn_key_t &key);

    void on_ring();
    bool is_expired(const http_conn_t &conn) const;

    std::string expired_error_message() const;
private:
    static const int64_t TIMER_RESOLUTION_MS = 5000;

    // Random number generator used for generating cryptographic connection IDs
    std::mt19937 key_generator;

    // We use a cryptographic hash with this unordered map to avoid timing
    // side channel attacks that might leak information about existing connection
    // keys.
    struct sha_hasher_t {
        size_t operator()(const conn_key_t &x) const;
    };
    std::unordered_map<conn_key_t, counted_t<http_conn_t>, sha_hasher_t> cache;
    repeating_timer_t http_timeout_timer;
    uint32_t http_timeout_sec;
};

class new_semaphore_in_line_t;
class query_handler_t {
public:
    virtual ~query_handler_t() { }

    virtual void run_query(ql::query_params_t *query_params,
                           ql::response_t *response_out,
                           signal_t *interruptor) = 0;
};

class query_server_t : public http_app_t {
public:
    query_server_t(
        rdb_context_t *rdb_ctx,
        const std::set<ip_address_t> &local_addresses,
        int port,
        query_handler_t *_handler,
        uint32_t http_timeout_sec,
        tls_ctx_t* tls_ctx);
    ~query_server_t();

    int get_port() const;

private:
    void make_error_response(bool is_draining,
                             const tcp_conn_t &conn,
                             const std::string &err,
                             ql::response_t *response_out);

    // For the client driver socket
    void handle_conn(const scoped_ptr_t<tcp_conn_descriptor_t> &nconn,
                     auto_drainer_t::lock_t);

    // This is templatized based on the wire protocol requested by the client
    template<class protocol_t>
    void connection_loop(tcp_conn_t *conn,
                         size_t max_concurrent_queries,
                         ql::query_cache_t *query_cache,
                         signal_t *interruptor);

    // For HTTP server
    void handle(const http_req_t &request,
                http_res_t *result,
                signal_t *interruptor);

    tls_ctx_t *tls_ctx;
    rdb_context_t *const rdb_ctx;
    query_handler_t *const handler;

    /* WARNING: The order here is fragile. */
    auto_drainer_t drainer;
    http_conn_cache_t http_conn_cache;
    scoped_ptr_t<tcp_listener_t> tcp_listener;

    int next_thread;
};

#endif /* CLIENT_PROTOCOL_SERVER_HPP_ */
