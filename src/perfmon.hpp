#ifndef __PERFMON_HPP__
#define __PERFMON_HPP__

#include <string>
#include <map>
#include <stdarg.h>
#include "config/alloc.hpp"
#include "utils2.hpp"
#include "containers/intrusive_list.hpp"

// Horrible hack because we define fail() as a macro
#undef fail
#include <sstream>
#define fail(...) _fail(__FILE__, __LINE__, __VA_ARGS__)

/* The perfmon (short for "PERFormance MONitor") is responsible for gathering data about
various parts of the server. */

/* A perfmon_stats_t is just a mapping from string keys to string values; it
stores statistics about the server. */

typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > std_string_t;

typedef std::map<std_string_t, std_string_t, std::less<std_string_t>,
    gnew_alloc<std::pair<std_string_t, std_string_t> > > perfmon_stats_t;

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
    perfmon_t(const char *name);
    ~perfmon_t();
    
    const char *name;
    virtual std_string_t get_value() = 0;   /* Should this be asynchronous? */
};

/* perfmon_counter_t is a perfmon_t that keeps a global counter that can be incremented
and decremented. (Internally, it keeps many individual counters for thread-safety.) */

class perfmon_counter_t :
    public perfmon_t
{
    int64_t values[MAX_CPUS];
    int64_t &get();
public:
    perfmon_counter_t(const char *name);
    void operator++(int) { get()++; }
    void operator+=(int64_t num) { get() += num; }
    void operator--(int) { get()--; }
    void operator-=(int64_t num) { get() -= num; }
    std_string_t get_value();
};

/* perfmon_thread_average_t is a perfmon_t that averages together values from
separate threads. */

class perfmon_thread_average_t :
    public perfmon_t
{
    int64_t values[MAX_CPUS];
public:
    perfmon_thread_average_t(const char *name);
    void set_value_for_this_thread(int64_t v);
    std_string_t get_value();
};

#endif /* __PERFMON_HPP__ */
