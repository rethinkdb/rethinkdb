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

struct perfmon_fsm_t :
    public home_cpu_mixin_t
{
    perfmon_callback_t *cb;
    perfmon_stats_t *dest;
    std::vector<perfmon_t::step_t*> steps;
    int messages_out;
    perfmon_fsm_t(perfmon_stats_t *dest) : cb(NULL), dest(dest) {
        steps.reserve(get_var_list().size());
        for (perfmon_t *p = get_var_list().head(); p; p = get_var_list().next(p)) {
            steps.push_back(p->begin());
        }
        messages_out = get_num_cpus();
        for (int i = 0; i < get_num_cpus(); i++) {
            do_on_cpu(i, this, &perfmon_fsm_t::visit);
        }
    }
    bool visit() {
        for (unsigned i = 0; i < get_var_list().size(); i++) {
            steps[i]->visit();
        }
        do_on_cpu(home_cpu, this, &perfmon_fsm_t::have_visited);
        return true;
    }
    bool have_visited() {
        messages_out--;
        if (messages_out == 0) {
            int i = 0;
            for (perfmon_t *p = get_var_list().head(); p; p = get_var_list().next(p)) {
                perfmon_t::step_t *s = steps[i++];
                (*dest)[p->name] = s->end();
            }
            if (cb) {
                cb->on_perfmon_stats();
                delete this;
            } else {
                /* Don't delete ourself; perfmon_get_stats() will delete us */
            }
        }
        return true;
    }
};

bool perfmon_get_stats(perfmon_stats_t *dest, perfmon_callback_t *cb) {
    
    perfmon_fsm_t *fsm = new perfmon_fsm_t(dest);
    if (fsm->messages_out == 0) {
        /* It has already finished */
        delete fsm;
        return true;
    } else {
        /* It has not finished yet */
        fsm->cb = cb;
        return false;
    }
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

struct perfmon_counter_step_t :
    public perfmon_t::step_t
{
    perfmon_counter_t *parent;
    int64_t values[MAX_CPUS];
    perfmon_counter_step_t(perfmon_counter_t *parent)
        : parent(parent) { }
    void visit() {
        values[get_cpu_id()] = parent->values[get_cpu_id()];
    }
    std::string end() {
        int64_t value = 0;
        for (int i = 0; i < get_num_cpus(); i++) value += values[i];
        delete this;
        std::stringstream s;
        s << value;
        return s.str();
    }
};

perfmon_t::step_t *perfmon_counter_t::begin() {
    
    return new perfmon_counter_step_t(this);
}

/* perfmon_thread_average_t */

perfmon_thread_average_t::perfmon_thread_average_t(const char *name)
    : perfmon_t(name) { }

void perfmon_thread_average_t::set_value_for_this_thread(int64_t v) {

    values[get_cpu_id()] = v;
}

struct perfmon_thread_average_step_t :
    public perfmon_t::step_t
{
    perfmon_thread_average_t *parent;
    int64_t values[MAX_CPUS];
    perfmon_thread_average_step_t(perfmon_thread_average_t *parent)
        : parent(parent) { }
    void visit() {
        values[get_cpu_id()] = parent->values[get_cpu_id()];
    }
    std::string end() {
        int64_t value = 0;
        for (int i = 0; i < get_num_cpus(); i++) value += values[i];
        value /= get_num_cpus();
        delete this;
        std::stringstream s;
        s << value;
        return s.str();
    }
};

perfmon_t::step_t *perfmon_thread_average_t::begin() {
    
    return new perfmon_thread_average_step_t(this);
}
