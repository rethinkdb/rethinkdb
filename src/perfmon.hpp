/* Please avoid #include'ing this file from other headers unless you absolutely
 * need to. Please #include "perfmon_types.hpp" instead. This helps avoid
 * potential circular dependency problems, since perfmons are used all over the
 * place but also depend on a significant chunk of our threading infrastructure
 * (by way of get_thread_id() & co).
 */
#ifndef __PERFMON_HPP__
#define __PERFMON_HPP__

#include <stdarg.h>

#include <map>
#include <limits>
#include <string>

#include "arch/core.hpp"
#include "utils.hpp"
#include "config/args.hpp"
#include "containers/intrusive_list.hpp"
#include "server/control.hpp"
#include "perfmon_types.hpp"

// Pad a value to the size of a cache line to avoid false sharing.
// TODO: This is implemented as a struct with subtraction rather than a union
// so that it gives an error when trying to pad a value bigger than
// CACHE_LINE_SIZE. If that's needed, this may have to be done differently.
// TODO: Use this in the rest of the perfmons, if it turns out to make any
// difference.

template<typename value_t>
struct cache_line_padded_t {
    value_t value;
    char padding[CACHE_LINE_SIZE - sizeof(value_t)];
};

/* The perfmon (short for "PERFormance MONitor") is responsible for gathering data about
various parts of the server. */

/* A perfmon_stats_t is just a mapping from string keys to string values; it
stores statistics about the server. */

typedef std::map<std::string, std::string> perfmon_stats_t;

/* perfmon_get_stats() collects all the stats about the server and puts them
into the given perfmon_stats_t object. It must be run in a coroutine and it blocks
until it is done. */

void perfmon_get_stats(perfmon_stats_t *dest, bool include_internal);

/* A perfmon_t represents a stat about the server.

To monitor something, declare a global variable that is an instance of a subclass of
perfmon_t and pass its name to the constructor. It is not safe to create a perfmon_t
after the server starts up because the global list is not thread-safe. */

class perfmon_t :
    public intrusive_list_node_t<perfmon_t>
{
public:
    bool internal;
public:
    perfmon_t(bool internal = true);
    virtual ~perfmon_t();
    
    /* To get a value from a given perfmon: Call begin_stats(). On each core, call the visit_stats()
    method with the pointer that was returned from begin_stats(). Then call end_stats() on the
    pointer on the same core that you called begin_stats() on.
    
    You usually want to call perfmon_get_stats() instead of calling these methods directly. */
    virtual void *begin_stats() = 0;
    virtual void visit_stats(void *) = 0;
    virtual void end_stats(void *, perfmon_stats_t *) = 0;
};

/* When `global_full_perfmon` is true, some perfmons will perform more elaborate stat
calculations, which might take longer but will produce more informative performance
stats. The command-line flag `--full-perfmon` sets `global_full_perfmon` to true. */

extern bool global_full_perfmon;


// Abstract perfmon subclass that implements perfmon tracking by combining per-thread values.
template<typename thread_stat_t, typename combined_stat_t = thread_stat_t>
struct perfmon_perthread_t
    : public perfmon_t
{
    perfmon_perthread_t(bool internal = true) : perfmon_t(internal) {};

    void *begin_stats() {
        return new thread_stat_t[get_num_threads()];
    }
    void visit_stats(void *data) {
        get_thread_stat(&(reinterpret_cast<thread_stat_t *>(data))[get_thread_id()]);
    }
    void end_stats(void *data, perfmon_stats_t *dest) {
        combined_stat_t combined = combine_stats(reinterpret_cast<thread_stat_t *>(data));
        output_stat(combined, dest);
        delete[] reinterpret_cast<thread_stat_t *>(data);
    }

  protected:
    virtual void get_thread_stat(thread_stat_t *) = 0;
    virtual combined_stat_t combine_stats(thread_stat_t *) = 0;
    virtual void output_stat(const combined_stat_t &, perfmon_stats_t *) = 0;
};

// Convenience macros for subclasses of perfmon_perthread_t that implement its pure virtual methods.
#define PERFMON_PERTHREAD_IMPL2(thread_stat_t, combined_stat_t)         \
    void get_thread_stat(thread_stat_t *);                              \
    combined_stat_t combine_stats(thread_stat_t *);                     \
    void output_stat(const combined_stat_t &, perfmon_stats_t *)

#define PERFMON_PERTHREAD_IMPL(thread_stat_t) PERFMON_PERTHREAD_IMPL2(thread_stat_t, thread_stat_t)

/* perfmon_counter_t is a perfmon_t that keeps a global counter that can be incremented
and decremented. (Internally, it keeps many individual counters for thread-safety.) */
class perfmon_counter_t :
    // TODO (rntz) does having the values when collecting stats be cache-padded matter?
    public perfmon_perthread_t<cache_line_padded_t<int64_t>, int64_t>
{
    friend class perfmon_counter_step_t;
  protected:
    typedef cache_line_padded_t<int64_t> padded_int64_t;
    std::string name;
    padded_int64_t thread_data[MAX_THREADS];
    int64_t &get();
    PERFMON_PERTHREAD_IMPL2(padded_int64_t, int64_t);
  public:
    explicit perfmon_counter_t(std::string name, bool internal = true);
    void operator++(int) { get()++; }
    void operator+=(int64_t num) { get() += num; }
    void operator--(int) { get()--; }
    void operator-=(int64_t num) { get() -= num; }
};

/* perfmon_sampler_t is a perfmon_t that keeps a log of events that happen. When something
happens, call the perfmon_sampler_t's record() method. The perfmon_sampler_t will retain that
record until 'length' ticks have passed. It will produce stats for the number of records in the
time period, the average record, and the min and max records. */

// need to use a namespace, not inner classes, so we can pass the auxiliary
// classes to the templated base classes
namespace perfmon_sampler {
    typedef double value_t;

    struct stats_t {
        int count;
        value_t sum, min, max;
        stats_t() : count(0), sum(0),
            min(std::numeric_limits<value_t>::max()),
            max(std::numeric_limits<value_t>::min())
            { }
        void record(value_t v) {
            count++;
            sum += v;
            if (count) {
                min = std::min(min, v);
                max = std::max(max, v);
            } else {
                min = max = v;
            }
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
}

class perfmon_sampler_t
    : public perfmon_perthread_t<perfmon_sampler::stats_t>
{
    typedef perfmon_sampler::value_t value_t;
    typedef perfmon_sampler::stats_t stats_t;
    struct thread_info_t {
        stats_t current_stats, last_stats;
        int current_interval;
    } thread_data[MAX_THREADS];
    PERFMON_PERTHREAD_IMPL(stats_t);
    void update(ticks_t now);

    std::string name;
    ticks_t length;
    bool include_rate;
  public:
    perfmon_sampler_t(std::string name, ticks_t length, bool include_rate = false, bool internal = true);
    void record(value_t value);
};

/* Tracks the mean and standard deviation of a sequence value in constant space
 * & time.
 */

// One-pass variance calculation algorithm/datastructure taken from
// http://www.cs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
struct stddev_t {
    stddev_t();
    stddev_t(size_t datapoints, float mean, float variance);

    void add(float value);
    size_t datapoints() const;
    float mean() const;
    float standard_deviation() const;
    float standard_variance() const;
    //stddev_t merge(const stddev_t &other);
    static stddev_t combine(size_t nelts, stddev_t *data);

  private:
    // N is the number of datapoints, M is the current mean, Q/N is
    // the standard variance, and sqrt(Q/N) is the standard
    // deviation. Read the paper for why it works or use algebra.
    //
    // (Note that this is a better scheme numerically than using the
    // classic calculator technique of storing sum(x), sum(x^2), n.)
    size_t N;
    float M, Q;
};

struct perfmon_stddev_t
    : public perfmon_perthread_t<stddev_t>
{
    // should be possible to make this a templated class if necessary
    explicit perfmon_stddev_t(std::string name, bool internal = true);
    void record(float value);

  protected:
    std::string name;
    PERFMON_PERTHREAD_IMPL(stddev_t);
  private:
    stddev_t thread_data[MAX_THREADS]; // TODO (rntz) should this be cache-line padded?
};

/* `perfmon_rate_monitor_t` keeps track of the number of times some event happens
per second. It is different from `perfmon_sampler_t` in that it does not associate
a number with each event, but you can record many events at once. For example, it
would be good for recording how fast bytes are sent over the network. */

class perfmon_rate_monitor_t
    : public perfmon_perthread_t<double>
{
private:
    struct thread_info_t {
        double current_count, last_count;
        int current_interval;
    } thread_data[MAX_THREADS];
    void update(ticks_t now);
    std::string name;
    ticks_t length;
    PERFMON_PERTHREAD_IMPL(double);
public:
    perfmon_rate_monitor_t(std::string name, ticks_t length, bool internal = true);
    void record(double value = 1.0);
};

/* perfmon_duration_sampler_t is a perfmon_t that monitors events that have a
 * starting and ending time. When something starts, call begin(); when
 * something ends, call end() with the same value as begin. It will produce
 * stats for the number of active events, the average length of an event, and
 * so on. If `global_full_perfmon` is false, it won't report any timing-related
 * stats because `get_ticks()` is rather slow.  
 *
 * Frequently we're in the case
 * where we'd like to have a single slow perfmon up, but don't want the other
 * ones, perfmon_duration_sampler_t has an ignore_global_full_perfmon
 * field on it, which when true makes it run regardless of --full-perfmon flag
 * this can also be enable and disabled at runtime. */

struct perfmon_duration_sampler_t 
    : public control_t
{

private:
    perfmon_counter_t active;
    perfmon_counter_t total;
    perfmon_sampler_t recent;
    bool ignore_global_full_perfmon;
public:
    perfmon_duration_sampler_t(std::string name, ticks_t length, bool internal = true, bool ignore_global_full_perfmon = false) 
        : control_t(std::string("pm_") + name + "_toggle", name + " toggle on and off", true),
          active(name + "_active_count", internal), total(name + "_total", internal), 
          recent(name, length, true, internal), ignore_global_full_perfmon(ignore_global_full_perfmon)
        { }
    void begin(ticks_t *v) {
        active++;
        total++;
        if (global_full_perfmon || ignore_global_full_perfmon) *v = get_ticks();
        else *v = 0;
    }
    void end(ticks_t *v) {
        active--;
        if (*v != 0) recent.record(ticks_to_secs(get_ticks() - *v));
    }

//Control interface used for enabling and disabling duration samplers at run time
public:
    std::string call(UNUSED int argc, UNUSED char **argv) {
        ignore_global_full_perfmon = !ignore_global_full_perfmon;
        if (ignore_global_full_perfmon) return std::string("Enabled\n");
        else                            return std::string("Disabled\n");
    }
};

/* perfmon_function_t is a perfmon for calling an arbitrary function to compute a stat. You should
not create such a perfmon at runtime; instead, declare it as a static variable and then construct
a perfmon_function_t::internal_function_t instance for it. This way things get called on the
right cores and if there are multiple internal_function_t instances the perfmon_function_t can
combine them by inserting commas. */
struct perfmon_function_t :
    public perfmon_t
{
public:
    class internal_function_t :
        public intrusive_list_node_t<internal_function_t>
    {
    public:
        internal_function_t(perfmon_function_t *p);
        virtual ~internal_function_t();
        virtual std::string compute_stat() = 0;
    private:
        perfmon_function_t *parent;
    };

private:
    friend class internal_function_t;
    std::string name;
    intrusive_list_t<internal_function_t> funs[MAX_THREADS];

public:
    perfmon_function_t(std::string name, bool internal = true)
        : perfmon_t(internal), name(name) {}
    ~perfmon_function_t() {}

    void *begin_stats();
    void visit_stats(void *data);
    void end_stats(void *data, perfmon_stats_t *dest);
};

struct block_pm_duration {
    ticks_t time;
    bool ended;
    perfmon_duration_sampler_t *pm;
    block_pm_duration(perfmon_duration_sampler_t *pm)
        : ended(false), pm(pm)
    {
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

#endif /* __PERFMON_HPP__ */
