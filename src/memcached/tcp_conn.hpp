// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef MEMCACHED_TCP_CONN_HPP_
#define MEMCACHED_TCP_CONN_HPP_

#include <set>

#include "arch/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "containers/scoped.hpp"
#include "memcached/protocol.hpp"
#include "memcached/stats.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"

class ip_address_t;

/* Serves memcache queries over the given TCP connection until the connection in question
is closed. */

void serve_memcache(tcp_conn_t *conn, namespace_interface_t<memcached_protocol_t> *nsi);

/* Listens for TCP connections on the given port and serves memcache queries over those
connections until the destructor is called. */

struct memcache_listener_t : public home_thread_mixin_debug_only_t {
    memcache_listener_t(const std::set<ip_address_t> &local_addresses,
                        int _port,
                        namespace_repo_t<memcached_protocol_t> *_ns_repo,
                        uuid_u _namespace_id,
                        perfmon_collection_t *_parent);
    ~memcache_listener_t();

    signal_t *get_bound_signal();

    int port;

private:
    const uuid_u namespace_id;
    namespace_repo_t<memcached_protocol_t> *ns_repo;

    int next_thread;

    perfmon_collection_t *parent;

    memcached_stats_t stats;

    /* We use this to make sure that all TCP connections stop when the
    `memcached_listener_t` is destroyed. */
    auto_drainer_t drainer;

    scoped_ptr_t<repeated_nonthrowing_tcp_listener_t> tcp_listener;

    void handle(auto_drainer_t::lock_t keepalive, const scoped_ptr_t<tcp_conn_descriptor_t>& conn);
};

#endif /* MEMCACHED_TCP_CONN_HPP_ */
