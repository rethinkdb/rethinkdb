// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef PROTOB_PROTOB_HPP_
#define PROTOB_PROTOB_HPP_

#include <set>
#include <map>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <openssl/evp.h>

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include "arch/address.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/timing.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "containers/archive/archive.hpp"
#include "containers/counted.hpp"
#include "http/http.hpp"
#include "perfmon/perfmon.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "utils.hpp"

class auth_key_t;
class auth_semilattice_metadata_t;
template <class> class semilattice_readwrite_view_t;

class rdb_context_t;
namespace ql {
class query_id_t;
class query_cache_t;
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
        size_t operator()(const conn_key_t &x) const {
            EVP_MD_CTX c;
            EVP_DigestInit(&c, EVP_sha256());
            EVP_DigestUpdate(&c, x.data(), x.size());
            unsigned char digest[EVP_MAX_MD_SIZE];
            unsigned int digest_size = 0;
            EVP_DigestFinal(&c, digest, &digest_size);
            rassert(digest_size >= sizeof(size_t));
            size_t res = 0;
            memcpy(&res, digest, std::min(sizeof(size_t),
                                          static_cast<size_t>(digest_size)));
            return res;
        }
    };
    std::unordered_map<conn_key_t, counted_t<http_conn_t>, sha_hasher_t> cache;
    repeating_timer_t http_timeout_timer;
    uint32_t http_timeout_sec;
};

class query_handler_t {
public:
    virtual ~query_handler_t() { }

    virtual void run_query(ql::query_id_t &&query_id,
                           const ql::protob_t<Query> &query,
                           Response *response_out,
                           ql::query_cache_t *query_cache,
                           signal_t *interruptor) = 0;
};

class query_server_t : public http_app_t {
public:
    query_server_t(
        rdb_context_t *rdb_ctx,
        const std::set<ip_address_t> &local_addresses,
        int port,
        query_handler_t *_handler,
        uint32_t http_timeout_sec);
    ~query_server_t();

    int get_port() const;

private:
    static std::string read_sized_string(tcp_conn_t *conn,
                                         size_t max_size,
                                         const std::string &length_error_msg,
                                         signal_t *interruptor);
    static auth_key_t read_auth_key(tcp_conn_t *conn, signal_t *interruptor);

    void make_error_response(bool is_draining,
                             const tcp_conn_t &conn,
                             const std::string &err,
                             Response *response_out);

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

    rdb_context_t *const rdb_ctx;
    query_handler_t *const handler;

    /* WARNING: The order here is fragile. */
    auto_drainer_t drainer;
    http_conn_cache_t http_conn_cache;
    scoped_ptr_t<tcp_listener_t> tcp_listener;

    int next_thread;
};

#endif /* PROTOB_PROTOB_HPP_ */
