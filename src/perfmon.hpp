#ifndef __PERFMON_HPP__
#define __PERFMON_HPP__

#include <string>
#include <map>
#include "config/alloc.hpp"
#include "utils2.hpp"

// Horrible hack because we define fail() as a macro
#pragma push_macro("fail")
#undef fail
#include <sstream>
#pragma pop_macro("fail")

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

class perfmon_watcher_t
{
public:
    perfmon_watcher_t(const char *name);
    ~perfmon_watcher_t();
    
    virtual std_string_t get_value() = 0;
    
    virtual std_string_t combine_value(std_string_t, std_string_t) {
        fail("Perfmon variable namespace collision for name %s", name);
    }

private:
    const char *name;
};

/* perfmon_var_t is a perfmon_watcher_t that just watches a variable. */

template<class var_t>
class perfmon_var_t :
    public perfmon_watcher_t
{
public:
    perfmon_var_t(const char *name, var_t *var)
        : perfmon_watcher_t(name), var(var) { }
    
    std_string_t get_value() {
        std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > s;
        s << *var;
        return s.str();
    }
    
    var_t *var;
};

/* perfmon_summed_var_t is a perfmon_watcher_t that watches a numeric variable and
sums its value across cores. */

template<class var_t>
class perfmon_summed_var_t :
    public perfmon_var_t<var_t>
{
public:
    std_string_t combine(std_string_t v1, std_string_t v2) {
        std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > s;
        s << (atoi(v1.c_str()) + atoi(v2.c_str()));
        return s.str();
    }
};

#endif /* __PERFMON_HPP__ */
