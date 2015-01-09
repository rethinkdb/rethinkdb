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

class cluster_semilattice_metadata_t;

enum class update_check_t {
    do_not_perform,
    perform,
};

class version_checker_t : private repeating_timer_callback_t {
public:
    typedef boost::shared_ptr<semilattice_readwrite_view_t
                              <cluster_semilattice_metadata_t> > metadata_ptr_t;
    version_checker_t(rdb_context_t *, version_checker_t::metadata_ptr_t,
                      const std::string &);
    void start_initial_check() {
        coro_t::spawn_sometime(std::bind(&version_checker_t::initial_check,
                                         this, drainer.lock()));
    }
private:
    void initial_check(auto_drainer_t::lock_t keepalive);
    void periodic_checkin(auto_drainer_t::lock_t keepalive);
    virtual void on_ring() {
        coro_t::spawn_sometime(std::bind(&version_checker_t::periodic_checkin,
                                         this, drainer.lock()));
    }
    void process_result(const http_result_t &);
    double cook(double);

    rdb_context_t *rdb_ctx;
    datum_string_t seen_version;
    version_checker_t::metadata_ptr_t metadata;
    std::string uname;
    auto_drainer_t drainer;
    repeating_timer_t timer;
};

#endif /* CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_ */
