// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_

#include "rdb_protocol/context.hpp"
#include "arch/timer.hpp"

struct http_result_t;

class version_checker_t : public timer_callback_t {
public:
    version_checker_t(rdb_context_t *, signal_t *);
    void initial_check();
    void periodic_checkin();
    virtual void on_timer() { periodic_checkin(); }
private:
    void process_result(const http_result_t &);
    double cook(double);

    rdb_context_t *rdb_ctx;
    signal_t *interruptor;
    datum_string_t seen_version;
};

#endif /* CLUSTERING_ADMINISTRATION_MAIN_VERSION_CHECK_HPP_ */
