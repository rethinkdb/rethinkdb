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
        for (unsigned i = 0; i < steps.size(); i++) {
            steps[i]->visit();
        }
        do_on_cpu(home_cpu, this, &perfmon_fsm_t::have_visited);
        return true;
    }
    bool have_visited() {
        messages_out--;
        if (messages_out == 0) {
            for (unsigned i = 0; i < steps.size(); i++) {
                steps[i]->end(dest);
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

perfmon_t::perfmon_t()
{
    get_var_list().push_back(this);
}

perfmon_t::~perfmon_t() {
    
    get_var_list().remove(this);
}

/* Number formatter */

template<class T>
std::string format(T value) {
    std::stringstream ss;
    ss.precision(8);
    ss << std::fixed << value;
    return ss.str();
}

/* perfmon_counter_t */

perfmon_counter_t::perfmon_counter_t(std::string name)
    : name(name)
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
    void end(perfmon_stats_t *dest) {
        int64_t value = 0;
        for (int i = 0; i < get_num_cpus(); i++) value += values[i];
        (*dest)[parent->name] = format(value);
        delete this;
    }
};

perfmon_t::step_t *perfmon_counter_t::begin() {
    
    return new perfmon_counter_step_t(this);
}

/* perfmon_sampler_t */

perfmon_sampler_t::perfmon_sampler_t(std::string name, ticks_t length)
    : length(length), name(name) { }

void perfmon_sampler_t::expire() {
    ticks_t now = get_ticks();
    std::deque<sample_t> &queue = values[get_cpu_id()];
    while (!queue.empty() && queue.front().timestamp + length < now) queue.pop_front();
}

void perfmon_sampler_t::record(value_t v) {
    expire();
    values[get_cpu_id()].push_back(sample_t(v, get_ticks()));
}

struct perfmon_sampler_step_t :
    public perfmon_t::step_t
{
    perfmon_sampler_t *parent;
    uint64_t counts[MAX_CPUS];
    perfmon_sampler_t::value_t values[MAX_CPUS], mins[MAX_CPUS], maxes[MAX_CPUS];
    perfmon_sampler_step_t(perfmon_sampler_t *parent)
        : parent(parent) { }
    void visit() {
        parent->expire();
        values[get_cpu_id()] = 0;
        counts[get_cpu_id()] = 0;
        for (std::deque<perfmon_sampler_t::sample_t>::iterator it = parent->values[get_cpu_id()].begin();
             it != parent->values[get_cpu_id()].end(); it++) {
            values[get_cpu_id()] += (*it).value;
            if (counts[get_cpu_id()] > 0) {
                mins[get_cpu_id()] = std::min(mins[get_cpu_id()], (*it).value);
                maxes[get_cpu_id()] = std::max(maxes[get_cpu_id()], (*it).value);
            } else {
                mins[get_cpu_id()] = (*it).value;
                maxes[get_cpu_id()] = (*it).value;
            }
            counts[get_cpu_id()]++;
        }
    }
    void end(perfmon_stats_t *dest) {
        perfmon_sampler_t::value_t value = 0;
        uint64_t count = 0;
        perfmon_sampler_t::value_t min, max;
        for (int i = 0; i < get_num_cpus(); i++) {
            value += values[i];
            count += counts[i];
            if (i > 0) {
                if (counts[i]) {
                    min = std::min(mins[i], min);
                    max = std::max(maxes[i], max);
                }
            } else {
                min = mins[i];
                max = maxes[i];
            }
        }
        (*dest)[parent->name + std::string("_nsamples")] = format(count);
        if (count > 0) {
            (*dest)[parent->name + std::string("_avg[secs]")] = format(value / count);
            (*dest)[parent->name + std::string("_min[secs]")] = format(min);
            (*dest)[parent->name + std::string("_max[secs]")] = format(max);
        } else {
            (*dest)[parent->name + std::string("_avg[secs]")] = "-";
            (*dest)[parent->name + std::string("_min[secs]")] = "-";
            (*dest)[parent->name + std::string("_max[secs]")] = "-";
        }
        delete this;
    }
};

perfmon_t::step_t *perfmon_sampler_t::begin() {
    
    return new perfmon_sampler_step_t(this);
}
