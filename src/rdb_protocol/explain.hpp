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

struct sample_info_t {
    sample_info_t();
    sample_info_t(ticks_t mean_duration_, size_t n_samples_);
    ticks_t mean_duration_;
    size_t n_samples_;

    RDB_DECLARE_ME_SERIALIZABLE;
};

class event_t {
public:
    enum type_t {
        START,      //The start of a task
        SPLIT,      //A task splitting in to parallel sub tasks
        SAMPLE,     //Sampling of repeated process
        STOP        //The end of a task
    };

    event_t();
    event_t(event_t::type_t type);
    event_t(const std::string &description);
    event_t(const sample_info_t sample_info);

    type_t type_;
    std::string description_; //used in START
    size_t n_parallel_jobs_; // used in SPLIT
    ticks_t when_; // used in START, SPLIT, STOP
    sample_info_t sample_info_; // used in SAMPLE

    RDB_DECLARE_ME_SERIALIZABLE;
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        event_t::type_t, int8_t,
        event_t::START, event_t::STOP);

typedef std::vector<event_t> event_log_t;

/* A trace_t wraps a task and makes it a bit easier to use. */
class trace_t {
public:
    trace_t();
    counted_t<const ql::datum_t> as_datum() const;
    event_log_t &&get_event_log();
private:
    friend class starter_t;
    friend class splitter_t;
    friend class sampler_t;
    void start(const std::string &description);
    void stop();
    void start_split();
    void stop_split(size_t n_parallel_jobs_, const event_log_t &event_log);
    void start_sample(event_log_t *sample_event_log);
    void stop_sample(const sample_info_t &sample_info);

    /* returns the event_log_t that we should put events in */
    event_log_t *event_log_target();
    event_log_t event_log_;
    /* redirected_event_log_ is used during sampling to send the events to the
     * sampler to be processed. */
    event_log_t *redirected_event_log_;
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

class sampler_t {
public:
    sampler_t(const std::string &description, trace_t *parent);
    sampler_t(const std::string &description, const scoped_ptr_t<trace_t> &parent);
    void new_sample();
    ~sampler_t();
private:
    trace_t *parent_;
    starter_t starter_;
    event_log_t event_log_;

    ticks_t total_time;
    size_t n_samples;
};

void print_event_log(const event_log_t &event_log);

} //namespace explain
#endif
