// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_

#include <functional>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "arch/runtime/coroutines.hpp"
#include "arch/timing.hpp"
#include "concurrency/auto_drainer.hpp"
#include "rdb_protocol/context.hpp"

struct http_result_t;
class server_config_client_t;
class table_meta_client_t;

enum class update_check_t {
    do_not_perform,
    perform,
};

class version_checker_t : private repeating_timer_callback_t {
public:
    version_checker_t(
        rdb_context_t *_rdb_ctx,
        const std::string &_uname,
        table_meta_client_t *_table_meta_client,
        server_config_client_t *_server_config_client);
private:
    void do_check(bool is_initial, auto_drainer_t::lock_t keepalive);
    virtual void on_ring() {
        coro_t::spawn_sometime(std::bind(&version_checker_t::do_check,
                                         this, false, drainer.lock()));
    }
    int cook(int);
    size_t count_servers();
    size_t count_tables();
    void process_result(const http_result_t &);

    rdb_context_t *const rdb_ctx;
    std::string const uname;
    table_meta_client_t *const table_meta_client;
    server_config_client_t *const server_config_client;

    datum_string_t seen_version;
    auto_drainer_t drainer;
    repeating_timer_t timer;
};

#endif /* CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_ */
