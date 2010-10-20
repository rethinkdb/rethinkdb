#ifndef __PERFMON_HPP__
#define __PERFMON_HPP__

#include <string>
#include <map>
#include "config/alloc.hpp"
#include "utils2.hpp"
#include "containers/intrusive_list.hpp"

// Horrible hack because we define fail() as a macro
#undef fail
#include <sstream>
#define fail(...) _fail(__FILE__, __LINE__, __VA_ARGS__)

/* The perfmon (short for "PERFormance MONitor") is responsible for gathering data about
various parts of the server. In general, it is not designed for speed; it passes a lot of
values by copy and formats a lot of strings. The idea is that perfmon will not be called
often enough for it to cause a significant performance hit. */

/* A perfmon_stats_t is just a mapping from string keys to string values; it
stores statistics about the server. */

typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > std_string_t;

typedef std::map<std_string_t, std_string_t, std::less<std_string_t>,
    gnew_alloc<std::pair<std_string_t, std_string_t> > > perfmon_stats_t;

/* perfmon_get_stats() collects all the stats about the server and puts them
into the given perfmon_stats_t object. */

struct perfmon_callback_t {
    virtual void on_perfmon_stats() = 0;
};

bool perfmon_get_stats(perfmon_stats_t *dest, perfmon_callback_t *cb);

/* A perfmon_watcher_t represents a stat about the server. Its constructor registers
it in a global map and its destructor deregisters it. Subclass it and override its
get_value() method to make a performance monitor variable. */

// Combines two values (from different watchers) into one, using some
// operation that is commutative and associative.
typedef std_string_t perfmon_combiner_t(std_string_t, std_string_t);

// A transformer of the final value built by some combination of
// perfmon_combiner_t calls.
typedef std_string_t perfmon_transformer_t(std_string_t);

class perfmon_watcher_t :
    public intrusive_list_node_t<perfmon_watcher_t>
{
public:
    perfmon_watcher_t(const char *name, perfmon_combiner_t *combiner = NULL,
                      perfmon_transformer_t *transformer = NULL);
    ~perfmon_watcher_t();
    
    virtual std_string_t get_value() = 0;

    const char *name;
    perfmon_combiner_t *combiner;
    perfmon_transformer_t *transformer;
};

/* perfmon_var_t is a perfmon_watcher_t that just watches a variable. */

template<class var_t>
class perfmon_var_t :
    public virtual perfmon_watcher_t
{
public:
    perfmon_var_t(const char *name, var_t *var, perfmon_combiner_t *combiner = NULL,
                  perfmon_transformer_t *transformer = NULL)
        : perfmon_watcher_t(name, combiner, transformer), var(var) { }

    // Gets a string representation of the value.  We work with string
    // representations of these things because the type system makes
    // it inconvenient or impossible to use different types.
    std_string_t get_value() {
        std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > s;
        s << *var;
        return s.str();
    }
    
    var_t *var;
};

/* If two perfmon watchers (on the same thread or on different ones) have the same name,
then the combiner function of one of them will be invoked to combine their values.
Which one's combiner function is invoked is arbitrary, so they should have the same
combiner function and it should be commutative and associative. */

// The sum combiner assumes that both strings represent integers; it returns their sum.
std_string_t perfmon_combiner_sum(std_string_t v1, std_string_t v2);

// The average combiner averages integer values. Because the combiner must be associative,
// it returns a string of the form "%d (average of %d)", and correctly handles strings of
// that form if they are passed as its argument.
std_string_t perfmon_combiner_average(std_string_t v1, std_string_t v2);

std_string_t perfmon_weighted_average_transformer(std_string_t intpair);

#endif /* __PERFMON_HPP__ */
