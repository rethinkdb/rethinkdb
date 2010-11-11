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
    
    /* To get a value from a given perfmon: Call begin(). On each core, call the visit() method
    of the step_t that was returned from begin(). Then call end() on the step_t on the same core
    that you called begin() on.
    
    You usually want to call perfmon_get_stats() instead of calling these methods directly. */
    struct step_t {
        virtual void visit() = 0;
        virtual void end(perfmon_stats_t *dest) = 0;
    };
    virtual step_t *begin() = 0;
};

/* perfmon_counter_t is a perfmon_t that keeps a global counter that can be incremented
and decremented. (Internally, it keeps many individual counters for thread-safety.) */

struct perfmon_counter_step_t;
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
    
    perfmon_t::step_t *begin();
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
    ticks_t length;
    std::string name;
public:
    perfmon_sampler_t(std::string name, ticks_t length);
    void record(value_t value);
    perfmon_t::step_t *begin();
};

/* perfmon_duration_sampler_t is a perfmon_t that monitors events that have a starting and ending
time. When something starts, call begin(); when something ends, call end() with the same value
as begin. It will produce stats for the number of active events, the average length of an event,
and the like. */

struct perfmon_duration_sampler_t {
private:
    perfmon_counter_t active;
    perfmon_sampler_t recent;
public:
    perfmon_duration_sampler_t(std::string name, ticks_t length)
        : active(name + "_count"), recent(name + "_time", length) { }
    void begin(ticks_t *v) {
        active++;
        *v = get_ticks();
    }
    void end(ticks_t *v) {
        active--;
        recent.record(ticks_to_secs(get_ticks() - *v));
    }
};

#endif /* __PERFMON_HPP__ */
