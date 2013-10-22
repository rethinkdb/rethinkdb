// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_EXPLAIN_HPP_
#define RDB_PROTOCOL_EXPLAIN_HPP_

#include <vector>

#include "containers/archive/stl_types.hpp"
#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "rpc/serialize_macros.hpp"
#include "utils.hpp"

namespace ql {
class datum_t;
} //namespace ql

namespace explain {

class event_t {
public:
    enum type_t {
        START,      //The start of a task
        SPLIT,      //A task splitting in to parallel sub tasks
        STOP        //The end of a task
    };

    event_t();
    event_t(event_t::type_t type);
    event_t(const std::string &description);

    type_t type_;
    std::string description_; //used in START
    size_t n_parallel_jobs_; // used in SPLIT
    ticks_t when_; // used in START, SPLIT, STOP

    RDB_DECLARE_ME_SERIALIZABLE;
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        event_t::type_t, int8_t,
        event_t::START, event_t::STOP);

typedef std::vector<event_t> event_log_t;

/* A trace_t wraps a task and makes it a bit easier to use. */
class trace_t {
public:
    counted_t<const ql::datum_t> as_datum() const;
    event_log_t &&get_event_log();
private:
    friend class starter_t;
    friend class splitter_t;
    void start(const std::string &description);
    void stop();
    void start_split();
    void stop_split(size_t n_parallel_jobs_, const event_log_t &event_log);
    event_log_t event_log_;
};

class starter_t {
public:
    starter_t(const std::string &description, trace_t *parent);
    starter_t(const std::string &description, const scoped_ptr_t<trace_t> &parent);
    ~starter_t();
private:
    trace_t *parent_;
};

class splitter_t {
public:
    splitter_t(trace_t *parent);
    splitter_t(const scoped_ptr_t<trace_t> &parent);
    void give_splits(size_t n_parallel_jobs_, const event_log_t &event_log);
    ~splitter_t();
private:
    trace_t *parent_;
    size_t n_parallel_jobs_;
    event_log_t event_log_;
    bool received_splits_;
};

void print_event_log(const event_log_t &event_log);

} //namespace explain
#endif
