#ifndef __PERFMON_HPP__
#define __PERFMON_HPP__

#include <string>
#include <map>
#include <deque>
#include <stdarg.h>
#include "utils2.hpp"
#include "config/args.hpp"
#include "containers/intrusive_list.hpp"

// Horrible hack because we define fail() as a macro
#undef fail
#include <sstream>
#define fail(...) _fail(__FILE__, __LINE__, __VA_ARGS__)

/* The perfmon (short for "PERFormance MONitor") is responsible for gathering data about
various parts of the server. */

/* A perfmon_stats_t is just a mapping from string keys to string values; it
stores statistics about the server. */

typedef std::map<std::string, std::string> perfmon_stats_t;

/* perfmon_get_stats() collects all the stats about the server and puts them
into the given perfmon_stats_t object. Its interface is asynchronous, but at
the moment it will always return true. */

struct perfmon_callback_t {
    virtual void on_perfmon_stats() = 0;
};

bool perfmon_get_stats(perfmon_stats_t *dest, perfmon_callback_t *cb);

/* A perfmon_t represents a stat about the server.

To monitor something, declare a global variable that is an instance of a subclass of
perfmon_t and pass its name to the constructor. It is not safe to create a perfmon_t
after the server starts up because the global list is not thread-safe. */

class perfmon_t :
    public intrusive_list_node_t<perfmon_t>
{
public:
    perfmon_t();
    ~perfmon_t();
    
    /* To get a value from a given perfmon: Call begin_stats(). On each core, call the visit_stats()
    method with the pointer that was returned from begin_stats(). Then call end_stats() on the
    pointer on the same core that you called begin_stats() on.
    
    You usually want to call perfmon_get_stats() instead of calling these methods directly. */
    virtual void *begin_stats() = 0;
    virtual void visit_stats(void *) = 0;
    virtual void end_stats(void *, perfmon_stats_t *) = 0;
};

/* perfmon_counter_t is a perfmon_t that keeps a global counter that can be incremented
and decremented. (Internally, it keeps many individual counters for thread-safety.) */

class perfmon_counter_t :
    public perfmon_t
{
    friend class perfmon_counter_step_t;
    int64_t values[MAX_CPUS];
    int64_t &get();
    std::string name;
public:
    perfmon_counter_t(std::string name);
    void operator++(int) { get()++; }
    void operator+=(int64_t num) { get() += num; }
    void operator--(int) { get()--; }
    void operator-=(int64_t num) { get() -= num; }
    
    void *begin_stats();
    void visit_stats(void *);
    void end_stats(void *, perfmon_stats_t *);
};

/* perfmon_sampler_t is a perfmon_t that keeps a log of events that happen. When something
happens, call the perfmon_sampler_t's record() method. The perfmon_sampler_t will retain that
record until 'length' ticks have passed. It will produce stats for the number of records in the
time period, the average record, and the min and max records. */

struct perfmon_sampler_step_t;
class perfmon_sampler_t :
    public perfmon_t
{
public:
    typedef double value_t;
private:
    friend class perfmon_sampler_step_t;
    struct sample_t {
        value_t value;
        ticks_t timestamp;
        sample_t(value_t v, time_t t) : value(v), timestamp(t) { }
    };
    std::deque<sample_t> values[MAX_CPUS];
    void expire();
    std::string name;
    ticks_t length;
    bool include_rate;
public:
    perfmon_sampler_t(std::string name, ticks_t length, bool include_rate = false);
    void record(value_t value);
    
    void *begin_stats();
    void visit_stats(void *);
    void end_stats(void *, perfmon_stats_t *);
};

/* perfmon_duration_sampler_t is a perfmon_t that monitors events that have a starting and ending
time. When something starts, call begin(); when something ends, call end() with the same value
as begin. It will produce stats for the number of active events, the average length of an event,
and the like. */

struct perfmon_duration_sampler_t {
private:
    perfmon_counter_t active;
    perfmon_counter_t total;
    perfmon_sampler_t recent;
public:
    perfmon_duration_sampler_t(std::string name, ticks_t length)
        : active(name + "_active_count"), total(name + "_total"), recent(name, length, true) { }
    void begin(ticks_t *v) {
        active++;
        total++;
        *v = get_ticks();
    }
    void end(ticks_t *v) {
        active--;
        recent.record(ticks_to_secs(get_ticks() - *v));
    }
};

#endif /* __PERFMON_HPP__ */
