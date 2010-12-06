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

/* Class that wraps a pthread spinlock.

TODO: This should live in the arch/ directory. */

class spinlock_t {
    pthread_spinlock_t l;
public:
    spinlock_t() {
        pthread_spin_init(&l, PTHREAD_PROCESS_PRIVATE);
    }
    ~spinlock_t() {
        pthread_spin_destroy(&l);
    }
    void lock() {
        int res = pthread_spin_lock(&l);
        guarantee_err(res == 0, "could not lock spin lock");
    }
    void unlock() {
        int res = pthread_spin_unlock(&l);
        guarantee_err(res == 0, "could not unlock spin lock");
    }
};

spinlock_t &get_var_lock() {
    
    /* To avoid static initialization fiasco */
    
    static spinlock_t lock;
    return lock;
}

/* This is the function that actually gathers the stats. It is illegal to create or destroy
perfmon_t objects while a perfmon_fsm_t is active. */

struct perfmon_fsm_t :
    public home_cpu_mixin_t
{
    perfmon_callback_t *cb;
    perfmon_stats_t *dest;
    std::vector<void*> data;
    int messages_out;
    explicit perfmon_fsm_t(perfmon_stats_t *dest) : cb(NULL), dest(dest) {
        data.reserve(get_var_list().size());
        for (perfmon_t *p = get_var_list().head(); p; p = get_var_list().next(p)) {
            data.push_back(p->begin_stats());
        }
        messages_out = get_num_cpus();
        for (int i = 0; i < get_num_cpus(); i++) {
            do_on_cpu(i, this, &perfmon_fsm_t::visit);
        }
    }
    bool visit() {
        int i = 0;
        for (perfmon_t *p = get_var_list().head(); p; p = get_var_list().next(p)) {
            p->visit_stats(data[i++]);
        }
        do_on_cpu(home_cpu, this, &perfmon_fsm_t::have_visited);
        return true;
    }
    bool have_visited() {
        messages_out--;
        if (messages_out == 0) {
            int i = 0;
            for (perfmon_t *p = get_var_list().head(); p; p = get_var_list().next(p)) {
                p->end_stats(data[i++], dest);
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
    get_var_lock().lock();
    get_var_list().push_back(this);
    get_var_lock().unlock();
}

perfmon_t::~perfmon_t() {
    
    get_var_lock().lock();
    get_var_list().remove(this);
    get_var_lock().unlock();
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

void *perfmon_counter_t::begin_stats() {
    return new int64_t[get_num_cpus()];
}

void perfmon_counter_t::visit_stats(void *data) {
    ((int64_t *)data)[get_cpu_id()] = get();
}

void perfmon_counter_t::end_stats(void *data, perfmon_stats_t *dest) {
    int64_t value = 0;
    for (int i = 0; i < get_num_cpus(); i++) value += ((int64_t *)data)[i];
    (*dest)[name] = format(value);
    delete[] (int64_t *)data;
}

/* perfmon_sampler_t */

perfmon_sampler_t::perfmon_sampler_t(std::string name, ticks_t length, bool include_rate)
    : name(name), length(length), include_rate(include_rate) { }

void perfmon_sampler_t::expire() {
    ticks_t now = get_ticks();
    std::deque<sample_t> &queue = values[get_cpu_id()];
    while (!queue.empty() && queue.front().timestamp + length < now) queue.pop_front();
}

void perfmon_sampler_t::record(value_t v) {
    expire();
    values[get_cpu_id()].push_back(sample_t(v, get_ticks()));
}

struct perfmon_sampler_step_t {
    uint64_t counts[MAX_CPUS];
    perfmon_sampler_t::value_t values[MAX_CPUS], mins[MAX_CPUS], maxes[MAX_CPUS];
};

void *perfmon_sampler_t::begin_stats() {
    return new perfmon_sampler_step_t;
}

void perfmon_sampler_t::visit_stats(void *data) {
    perfmon_sampler_step_t *d = (perfmon_sampler_step_t *)data;
    expire();
    d->values[get_cpu_id()] = 0;
    d->counts[get_cpu_id()] = 0;
    for (std::deque<perfmon_sampler_t::sample_t>::iterator it = values[get_cpu_id()].begin();
         it != values[get_cpu_id()].end(); it++) {
        d->values[get_cpu_id()] += (*it).value;
        if (d->counts[get_cpu_id()] > 0) {
            d->mins[get_cpu_id()] = std::min(d->mins[get_cpu_id()], (*it).value);
            d->maxes[get_cpu_id()] = std::max(d->maxes[get_cpu_id()], (*it).value);
        } else {
            d->mins[get_cpu_id()] = (*it).value;
            d->maxes[get_cpu_id()] = (*it).value;
        }
        d->counts[get_cpu_id()]++;
    }
}

void perfmon_sampler_t::end_stats(void *data, perfmon_stats_t *dest) {
    perfmon_sampler_step_t *d = (perfmon_sampler_step_t *)data;
    perfmon_sampler_t::value_t value = 0;
    uint64_t count = 0;
    perfmon_sampler_t::value_t min = 0, max = 0;   /* Initializers to make GCC shut up */
    bool have_any = false;
    for (int i = 0; i < get_num_cpus(); i++) {
        value += d->values[i];
        count += d->counts[i];
        if (d->counts[i]) {
            if (have_any) {
                min = std::min(d->mins[i], min);
                max = std::max(d->maxes[i], max);
            } else {
                min = d->mins[i];
                max = d->maxes[i];
                have_any = true;
            }
        }
    }
    if (have_any) {
        (*dest)[name + "_avg"] = format(value / count);
        (*dest)[name + "_min"] = format(min);
        (*dest)[name + "_max"] = format(max);
    } else {
        (*dest)[name + "_avg"] = "-";
        (*dest)[name + "_min"] = "-";
        (*dest)[name + "_max"] = "-";
    }
    if (include_rate) {
        (*dest)[name + "_persec"] = format(count / ticks_to_secs(length));
    }
    delete d;
};

