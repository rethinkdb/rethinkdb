// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_

#include "errors.hpp"
#include <functional>
#include <boost/shared_ptr.hpp>

#include "rdb_protocol/context.hpp"
#include "arch/timing.hpp"
#include "arch/runtime/coroutines.hpp"

struct http_result_t;

class cluster_semilattice_metadata_t;

class version_checker_t : public repeating_timer_callback_t {
public:
    typedef boost::shared_ptr<semilattice_readwrite_view_t
                              <cluster_semilattice_metadata_t> > metadata_ptr_t;
    version_checker_t(rdb_context_t *, signal_t *, version_checker_t::metadata_ptr_t,
                      const std::string &);
    void initial_check();
    void periodic_checkin(auto_drainer_t::lock_t lock);
    virtual void on_ring() {
        coro_t::spawn_sometime(std::bind(&version_checker_t::periodic_checkin,
                                         this, drainer.lock()));
    }
private:
    void process_result(const http_result_t &);
    double cook(double);

    rdb_context_t *rdb_ctx;
    signal_t *interruptor;
    datum_string_t seen_version;
    version_checker_t::metadata_ptr_t metadata;
    std::string uname;
    auto_drainer_t drainer;
};

#endif /* CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_ */
