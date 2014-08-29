// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PROFILE_HPP_
#define RDB_PROTOCOL_PROFILE_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/variant.hpp>

#include "containers/counted.hpp"
#include "containers/scoped.hpp"
#include "rpc/serialize_macros.hpp"
#include "time.hpp"

namespace ql {
class datum_t;
} //namespace ql

namespace profile {

struct start_t {
    start_t();
    explicit start_t(const std::string &description);
    std::string description_;
    ticks_t when_;
};

RDB_DECLARE_SERIALIZABLE(start_t);

struct split_t {
    split_t();
    explicit split_t(size_t n_parallel_jobs);

    size_t n_parallel_jobs_;
};

RDB_DECLARE_SERIALIZABLE(split_t);

struct sample_t {
    sample_t();
    sample_t(const std::string &description, ticks_t mean_duration, size_t n_samples);

    std::string description_;
    ticks_t mean_duration_;
    size_t n_samples_;
};

RDB_DECLARE_SERIALIZABLE(sample_t);

struct stop_t {
    stop_t();
    ticks_t when_;
};

RDB_DECLARE_SERIALIZABLE(stop_t);

typedef boost::variant<start_t, split_t, sample_t, stop_t> event_t;

typedef std::vector<event_t> event_log_t;

/* A trace_t contains an event_log_t and provides private methods for adding
 * events to it. These methods are leveraged by the instruments. */
class trace_t {
public:
    trace_t();
    ql::datum_t as_datum() const;
    event_log_t extract_event_log() RVALUE_THIS;
private:
    friend class starter_t;
    friend class splitter_t;
    friend class sampler_t;
    friend class disabler_t;
    void start(const std::string &description);
    void stop();
    void start_split();
    void stop_split(size_t n_parallel_jobs_, const event_log_t &event_log);
    void start_sample(event_log_t *sample_event_log);
    void stop_sample(const std::string &description, ticks_t mean_duration,
        size_t n_samples, event_log_t *sample_event_log);
    void stop_sample(event_log_t *sample_event_log);
    void disable();
    void enable();

    /* returns the event_log_t that we should put events in */
    event_log_t *event_log_target();
    event_log_t event_log_;
    /* redirected_event_log_ is used during sampling to send the events to the
     * sampler to be processed. */
    event_log_t *redirected_event_log_;
    size_t disabled_ref_count_;
    bool disabled();

    DISABLE_COPYING(trace_t);
};

/* These are the instruments for adding profiling to code. You construct this
 * objects at various places in your code such that their lifetimes correspond
 * to the period of time during which you are performing an operation. */

/* starter_t is used to record a task. It records events when it is constructed
 * and when it is destroyed. Example:
 *
 *  {
 *      starter_t starter("Performing task foo.", trace);
 *
 *      Perform the task foo in here
 *  }
 */
class starter_t {
public:
    starter_t(const std::string &description, trace_t *parent);
    starter_t(const std::string &description, const scoped_ptr_t<trace_t> &parent);
    ~starter_t();
private:
    void init(const std::string &description, trace_t *parent);
    trace_t *parent_;
};

/* splitter_t is used to record several tasks happening in parallel. For each
 * parallel task you get back you need to get their event logs back,
 * concatenate them and pass them to the give_splits method. Example:
 * {
 *     splitter_t splitter(trace);
 *     event_log_t log1, log2;
 *     coro_t::spawn_*(&perform_task_1, &log1);
 *     coro_t::spawn_*(&perform_task_2, &log2);
 *
 *     wait_for_tasks_to_complete();
 *     log1.insert(log2.begin(), log2.end(), log1.end());
 *     splitter.give_splits(2, log1);
 * }
 *
 * This is used for reads and writes from and to the shards in the query
 * language layer.
 */
class splitter_t {
public:
    explicit splitter_t(trace_t *parent);
    explicit splitter_t(const scoped_ptr_t<trace_t> &parent);
    void give_splits(size_t n_parallel_jobs_, const event_log_t &event_log);
    ~splitter_t();
private:
    void init(trace_t *parent);
    trace_t *parent_;
    size_t n_parallel_jobs_;
    event_log_t event_log_;
};

/* sampler_t is used when you want to record a repeated event that would
 * otherwise swamp the event log. When it is constructed it begins sampling,
 * between samples you should call the new_sample method. Example:
 * {
 *     sampler_t sampler("Reduce elements", trace);
 *     datum_t res = stream.next();
 *     sampler.new_sample();
 *
 *     while (auto v = stream.next()) {
 *         res = reduce(res, v);
 *         sampler.new_sample();
 *     }
 * }
 *
 * This is used for map and reduce type things in the query language.
 *
 * Note: the sampler tries to be a bit flexible with where you put calls to
 * new_sample(). You need to call it between when the things you sample want to
 * happen. Calling it before the first sample and after the last sample is
 * acceptable but optional. */

class sampler_t {
public:
    sampler_t(const std::string &description, trace_t *parent);
    sampler_t(const std::string &description, const scoped_ptr_t<trace_t> &parent);
    void new_sample();
    ~sampler_t();
private:
    void init(const std::string &description, trace_t *parent);
    trace_t *parent_;
    event_log_t event_log_;

    std::string description_;
    ticks_t total_time_;
    size_t n_samples_;
};

/* disabler_t is used when you don't want a section of code to be profiled.
 * Right now this is used for parts of the code that would otherwise use
 * trace_t illegally due to their reentrancy. */
class disabler_t {
public:
    explicit disabler_t(trace_t *parent);
    explicit disabler_t(const scoped_ptr_t<trace_t> &parent);
    ~disabler_t();
private:
    void init(trace_t *parent);
    trace_t *parent_;
};

void print_event_log(const event_log_t &event_log);

}  // namespace profile

#endif  // RDB_PROTOCOL_PROFILE_HPP_
