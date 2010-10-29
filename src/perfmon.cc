#include "perfmon.hpp"
#include "arch/arch.hpp"
#include "utils.hpp"
#include <stdarg.h>

/* The var list keeps track of all of the perfmon_t objects. */

intrusive_list_t<perfmon_t> &get_var_list() {
    
    /* Getter function so that we can be sure that var_list is initialized before it is needed,
    as advised by the C++ FAQ. Otherwise, a perfmon_t might be initialized before the var list
    was initialized. */
    
    static intrusive_list_t<perfmon_t> var_list;
    return var_list;
}

/* This is the function that actually gathers the stats. */

bool perfmon_get_stats(perfmon_stats_t *dest, perfmon_callback_t *cb) {
    
    for (perfmon_t *p = get_var_list().head(); p; p = get_var_list().next(p)) {
        
        (*dest)[p->name] = p->get_value();
    }
    
    return true;
}

/* Constructor and destructor register and deregister the perfmon.

Right now, it is illegal to make perfmon_t objects that are not static variables, so
we don't have to worry about locking the var list. If we locked it, then we could
create and destroy perfmon_ts at runtime. */

perfmon_t::perfmon_t(const char *n)
    : name(n)
{
    get_var_list().push_back(this);
}

perfmon_t::~perfmon_t() {
    
    get_var_list().remove(this);
}

/* perfmon_counter_t */

perfmon_counter_t::perfmon_counter_t(const char *name)
    : perfmon_t(name)
{
    for (int i = 0; i < MAX_CPUS; i++) values[i] = 0;
}

int64_t &perfmon_counter_t::get() {
    return values[get_cpu_id()];
}

std_string_t perfmon_counter_t::get_value() {
    
    /* Don't bother sending messages to the other CPUs to request values because it would
    be too much trouble. Instead, just copy the value directly even though another core might
    be modifying it. It's unlikely that this will cause a problem, and besides if it does the
    worst that will happen is the stats will be nonsense. */
    
    int64_t value = 0;
    for (int i = 0; i < MAX_CPUS; i++) value += values[i];
    
    std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > s;
    s << value;
    return s.str();
}

/* perfmon_thread_average_t */

perfmon_thread_average_t::perfmon_thread_average_t(const char *name)
    : perfmon_t(name) { }

void perfmon_thread_average_t::set_value_for_this_thread(int64_t v) {

    values[get_cpu_id()] = v;
}

std_string_t perfmon_thread_average_t::get_value() {

    int64_t value = 0;
    for (int i = 0; i < get_num_cpus(); i++) value += values[i];
    value /= get_num_cpus();
    
    std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > s;
    s << value;
    return s.str();
}
