// Copyright 2010-2014 RethinkDB, all rights reserved.
/* Please avoid #include'ing this file from other headers unless you absolutely
 * need to. Please #include "perfmon/types.hpp" instead. This helps avoid
 * potential circular dependency problems, since perfmons are used all over the
 * place but also depend on a significant chunk of our threading infrastructure
 * (by way of get_thread_id() & co).
 */
#ifndef PERFMON_PERFMON_HPP_
#define PERFMON_PERFMON_HPP_

#include <algorithm>
#include <limits>
#include <string>
#include <map>
#include <memory>

#include "concurrency/cache_line_padded.hpp"
#include "config/args.hpp"
#include "perfmon/types.hpp"
#include "perfmon/core.hpp"
#include "time.hpp"

// Some arch/runtime declarations.
int get_num_threads();
threadnum_t get_thread_id();

/* When `global_full_perfmon` is true, some perfmons will perform more
 * elaborate stat calculations, which might take longer but will produce more
 * informative performance stats. The compile time option FULL_PERFMON sets
 * `global_full_perfmon` to true.
 */
extern bool global_full_perfmon;

// Abstract perfmon subclass that implements perfmon tracking by combining per-thread values.
template<typename thread_stat_t, typename combined_stat_t = thread_stat_t>
struct perfmon_perthread_t : public perfmon_t {
    perfmon_perthread_t() { }

    void *begin_stats() {
        return new thread_stat_t[get_num_threads()];
    }
    void visit_stats(void *data) {
        get_thread_stat(&(static_cast<thread_stat_t *>(data))[get_thread_id().threadnum]);
    }
    ql::datum_t end_stats(void *v_data) {
        std::unique_ptr<thread_stat_t[]> data(static_cast<thread_stat_t *>(v_data));
        combined_stat_t combined = combine_stats(data.get());
        return output_stat(combined);
    }

protected:
    virtual void get_thread_stat(thread_stat_t *) = 0;
    virtual combined_stat_t combine_stats(const thread_stat_t *) = 0;
    virtual ql::datum_t output_stat(const combined_stat_t &) = 0;
};

/* perfmon_counter_t is a perfmon_t that keeps a global counter that can be
 * incremented and decremented. (Internally, it keeps many individual counters
 * for thread-safety.)
 */
class perfmon_counter_t : public perfmon_perthread_t<cache_line_padded_t<int64_t>, int64_t> {
    friend class perfmon_counter_step_t;
protected:
    typedef cache_line_padded_t<int64_t> padded_int64_t;
    padded_int64_t *thread_data;

    int64_t &get();

    void get_thread_stat(padded_int64_t *);
    int64_t combine_stats(const padded_int64_t *);
    ql::datum_t output_stat(const int64_t&);
public:
    perfmon_counter_t();
    virtual ~perfmon_counter_t();
    void operator++() { get()++; }
    void operator+=(int64_t num) { get() += num; }
    void operator--() { get()--; }
    void operator-=(int64_t num) { get() -= num; }
};

class scoped_perfmon_counter_t {
public:
    explicit scoped_perfmon_counter_t(perfmon_counter_t *_counter);
    ~scoped_perfmon_counter_t();
private:
    perfmon_counter_t *counter;
    DISABLE_COPYING(scoped_perfmon_counter_t);
};

/* perfmon_sampler_t is a perfmon_t that keeps a log of events that happen.
 * When something happens, call the perfmon_sampler_t's record() method. The
 * perfmon_sampler_t will retain that record until 'length' ticks have passed.
 * It will produce stats for the number of records in the time period, the
 * average record, and the min and max records.
 */

// need to use a namespace, not inner classes, so we can pass the auxiliary
// classes to the templated base classes
namespace perfmon_sampler {

struct stats_t {
    int count;
    double sum, min, max;
    stats_t() : count(0), sum(0),
        min(std::numeric_limits<double>::max()),
        max(std::numeric_limits<double>::min())
        { }
    void record(double v) {
        count++;
        sum += v;
        min = std::min(min, v);
        max = std::max(max, v);
    }
    void aggregate(const stats_t &s) {
        count += s.count;
        sum += s.sum;
        if (s.count) {
            min = std::min(min, s.min);
            max = std::max(max, s.max);
        }
    }
};

}   /* namespace perfmon_sampler */

class perfmon_sampler_t : public perfmon_perthread_t<perfmon_sampler::stats_t> {
    typedef perfmon_sampler::stats_t stats_t;
    struct thread_info_t {
        stats_t current_stats, last_stats;
        int current_interval;
    };

    thread_info_t *thread_data;

    void get_thread_stat(stats_t *);
    stats_t combine_stats(const stats_t *);
    ql::datum_t output_stat(const stats_t&);

    void update(ticks_t now);

    ticks_t length;
    bool include_rate;
public:
    perfmon_sampler_t(ticks_t _length, bool _include_rate);
    virtual ~perfmon_sampler_t();
    void record(double value);
};

// One-pass variance calculation algorithm/datastructure taken from
// http://www.cs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
struct stddev_t {
    stddev_t();
    stddev_t(size_t datapoints, double mean, double variance);

    void add(double value);
    size_t datapoints() const;
    double mean() const;
    double standard_deviation() const;
    double standard_variance() const;
    static stddev_t combine(size_t nelts, const stddev_t *data);

private:
    // N is the number of datapoints, M is the current mean, Q/N is
    // the standard variance, and sqrt(Q/N) is the standard
    // deviation. Read the paper for why it works or use algebra.
    //
    // (Note that this is a better scheme numerically than using the
    // classic calculator technique of storing sum(x), sum(x^2), n.)
    size_t N;
    double M, Q;
};

/* Tracks the mean and standard deviation of a sequence value in constant space
 * & time.
 */
struct perfmon_stddev_t : public perfmon_perthread_t<stddev_t> {
private:
public:
    // should be possible to make this a templated class if necessary
    perfmon_stddev_t();
    void record(double value);

protected:
    void get_thread_stat(stddev_t *);
    stddev_t combine_stats(const stddev_t *);
    ql::datum_t output_stat(const stddev_t&);
private:
    cache_line_padded_t<stddev_t> thread_data[MAX_THREADS];
};

/* `perfmon_rate_monitor_t` keeps track of the number of times some event
 * happens per second. It is different from `perfmon_sampler_t` in that it does
 * not associate a number with each event, but you can record many events at
 * once. For example, it would be good for recording how fast bytes are sent
 * over the network.
 */
class perfmon_rate_monitor_t : public perfmon_perthread_t<double> {
private:
    struct thread_info_t {
        double current_count, last_count;
        int current_interval;

        thread_info_t() : current_count(0), last_count(0), current_interval(1) { }
    };

    cache_line_padded_t<thread_info_t> thread_data[MAX_THREADS];
    void update(ticks_t now);
    ticks_t length;

    void get_thread_stat(double *);
    double combine_stats(const double *);
    ql::datum_t output_stat(const double&);
public:
    explicit perfmon_rate_monitor_t(ticks_t length);
    void record(double value = 1.0);
};

/* perfmon_duration_sampler_t is a perfmon_t that monitors events that have a
 * starting and ending time. When something starts, call begin(); when
 * something ends, call end() with the same value as begin. It will produce
 * stats for the number of active events, the average length of an event, and
 * so on. If `global_full_perfmon` is false, it won't report any timing-related
 * stats because `get_ticks()` is rather slow.
 *
 * Frequently we're in the case where we'd like to have a single slow perfmon
 * up, but don't want the other ones, perfmon_duration_sampler_t has an
 * ignore_global_full_perfmon field on it, which when true makes it run
 * regardless of FULL_PERFMON flag this can also be enabled and disabled at
 * runtime.
 */
struct perfmon_duration_sampler_t : public perfmon_t {
private:
    perfmon_collection_t stat;
    perfmon_counter_t active;
    perfmon_counter_t total;
    perfmon_sampler_t recent;
    perfmon_membership_t active_membership;
    perfmon_membership_t total_membership;
    perfmon_membership_t recent_membership;

    bool ignore_global_full_perfmon;
public:
    explicit perfmon_duration_sampler_t(ticks_t length, bool _ignore_global_full_perfmon = false);
    void begin(ticks_t *v);
    void end(ticks_t *v);

    void *begin_stats();
    void visit_stats(void *data);
    ql::datum_t end_stats(void *data);

public:
    //Control interface used for enabling and disabling duration samplers at run time
    std::string call(UNUSED int argc, UNUSED char **argv);
};

struct block_pm_duration {
    ticks_t time;
    bool ended;
    perfmon_duration_sampler_t *pm;
    explicit block_pm_duration(perfmon_duration_sampler_t *_pm)
        : ended(false), pm(_pm) {
        pm->begin(&time);
    }
    void end() {
        rassert(!ended);
        ended = true;
        pm->end(&time);
    }
    ~block_pm_duration() {
        if (!ended) end();
    }
};

#endif /* PERFMON_PERFMON_HPP_ */
