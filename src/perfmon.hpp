#ifndef __PERFMON_HPP__
#define __PERFMON_HPP__

#include <string>
#include <map>
#include "config/alloc.hpp"
#include "utils.hpp"

// Horrible hack because we define fail() as a macro
#pragma push_macro("fail")
#undef fail
#include <sstream>
#pragma pop_macro("fail")

class perfmon_watcher_t;
class perfmon_fsm_t;

typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > std_string_t;

typedef std::map<std_string_t, std_string_t, std::less<std_string_t>,
    gnew_alloc<std::pair<std_string_t, std_string_t> > > perfmon_stats_t;

struct perfmon_callback_t {
    virtual void on_perfmon_stats() = 0;
};

class perfmon_controller_t {
    friend class perfmon_watcher_t;
    friend class one_perfmon_fsm_t;
    friend class perfmon_fsm_t;
    
public:
    perfmon_controller_t();
    ~perfmon_controller_t();

    bool get_stats(perfmon_stats_t *dest, perfmon_callback_t *cb);
    
    static perfmon_controller_t *controller;
    
private:    
    typedef std::map<std_string_t, perfmon_watcher_t*, std::less<std_string_t>,
        gnew_alloc<std::pair<std_string_t, perfmon_watcher_t*> > > var_map_t;
    var_map_t *var_maps[MAX_CPUS];
};

class perfmon_watcher_t :
    public home_cpu_mixin_t
{
public:
    perfmon_watcher_t(const char *name);
    ~perfmon_watcher_t();
    virtual std_string_t get_value() = 0;

private:
    const char *name;
};

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

#endif /* __PERFMON_HPP__ */
