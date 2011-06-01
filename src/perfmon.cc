#include "perfmon.hpp"
#include "concurrency/pmap.hpp"
#include "arch/arch.hpp"
#include "utils.hpp"
#include <stdarg.h>
#include <math.h>

/* The var list keeps track of all of the perfmon_t objects. */

intrusive_list_t<perfmon_t> &get_var_list() {
    /* Getter function so that we can be sure that var_list is initialized before it is needed,
    as advised by the C++ FAQ. Otherwise, a perfmon_t might be initialized before the var list
    was initialized. */
    
    static intrusive_list_t<perfmon_t> var_list;
    return var_list;
}

/* The var lock protects the var list when it is being modified. In theory, this should all work
automagically because the constructor of every perfmon_t calls get_var_lock(), causing the var lock
to be constructed before the first perfmon, so it is destroyed after the last perfmon. */

spinlock_t &get_var_lock() {
    /* To avoid static initialization fiasco */
    
    static spinlock_t lock;
    return lock;
}


/* This is the function that actually gathers the stats. It is illegal to create or destroy
perfmon_t objects while perfmon_get_stats is active. */

void co_perfmon_visit(int thread, const std::vector<void*> &data, bool include_internal) {
    on_thread_t moving(thread);
    int i = 0;
    for (perfmon_t *p = get_var_list().head(); p; p = get_var_list().next(p)) {
        if (!p->internal || include_internal)
            p->visit_stats(data[i++]);
    }
}


void perfmon_get_stats(perfmon_stats_t *dest, bool include_internal) {

    std::vector<void*> data;

    data.reserve(get_var_list().size());
    for (perfmon_t *p = get_var_list().head(); p; p = get_var_list().next(p)) {
        if (!p->internal || include_internal) 
            data.push_back(p->begin_stats());
    }

    pmap(get_num_threads(), boost::bind(&co_perfmon_visit, _1, data, include_internal));

    int i = 0;
    for (perfmon_t *p = get_var_list().head(); p; p = get_var_list().next(p)) {
        if (!p->internal || include_internal)
            p->end_stats(data[i++], dest);
    }
}

/* Constructor and destructor register and deregister the perfmon. */

perfmon_t::perfmon_t(bool internal)
    : internal(internal)
{
    spinlock_acq_t acq(&get_var_lock());
    get_var_list().push_back(this);
}

perfmon_t::~perfmon_t() {
    spinlock_acq_t acq(&get_var_lock());
    get_var_list().remove(this);
}

bool global_full_perfmon = false;

/* perfmon_counter_t */

perfmon_counter_t::perfmon_counter_t(std::string name, bool internal)
    : perfmon_t(internal), name(name)
{
    for (int i = 0; i < MAX_THREADS; i++) values[i].value = 0;
}

int64_t &perfmon_counter_t::get() {
    return values[get_thread_id()].value;
}

void *perfmon_counter_t::begin_stats() {
    return new cache_line_padded_t<int64_t>[get_num_threads()];
}

void perfmon_counter_t::visit_stats(void *data) {
    ((cache_line_padded_t<int64_t> *)data)[get_thread_id()].value = get();
}

void perfmon_counter_t::end_stats(void *data, perfmon_stats_t *dest) {
    int64_t value = 0;
    for (int i = 0; i < get_num_threads(); i++) value += ((cache_line_padded_t<int64_t> *)data)[i].value;
    (*dest)[name] = format(value);
    delete[] (cache_line_padded_t<int64_t> *)data;
}

/* perfmon_sampler_t */

perfmon_sampler_t::perfmon_sampler_t(std::string name, ticks_t length, bool include_rate, bool internal)
    : perfmon_t(internal), name(name), length(length), include_rate(include_rate)
{
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_info[i].current_interval = get_ticks() / length;
    }
}

void perfmon_sampler_t::update(ticks_t now) {
    int interval = now / length;
    thread_info_t *thread = &thread_info[get_thread_id()];

    if (thread->current_interval == interval) {
        /* We're up to date; nothing to do */
    } else if (thread->current_interval + 1 == interval) {
        /* We're one step behind */
        thread->last_stats = thread->current_stats;
        thread->current_stats = stats_t();
        thread->current_interval++;
    } else {
        /* We're more than one step behind */
        thread->last_stats = thread->current_stats = stats_t();
        thread->current_interval = interval;
    }
}

void perfmon_sampler_t::record(value_t v) {
    ticks_t now = get_ticks();
    update(now);
    thread_info_t *thread = &thread_info[get_thread_id()];
    thread->current_stats.record(v);
}

void *perfmon_sampler_t::begin_stats() {
    return new stats_t[get_num_threads()];
}

void perfmon_sampler_t::visit_stats(void *data) {
    update(get_ticks());
    /* Return last_stats instead of current_stats so that we can give a complete interval's
    worth of stats. We might be halfway through an interval, in which case current_stats will
    only have half an interval worth. */
    ((stats_t *)data)[get_thread_id()] = thread_info[get_thread_id()].last_stats;
}

void perfmon_sampler_t::end_stats(void *data, perfmon_stats_t *dest) {
    stats_t *stats = (stats_t *)data;
    stats_t aggregated;
    for (int i = 0; i < get_num_threads(); i++) {
        aggregated.aggregate(stats[i]);
    }

    if (aggregated.count > 0) {
        (*dest)[name + "_avg"] = format(aggregated.sum / aggregated.count);
        (*dest)[name + "_min"] = format(aggregated.min);
        (*dest)[name + "_max"] = format(aggregated.max);
    } else {
        (*dest)[name + "_avg"] = "-";
        (*dest)[name + "_min"] = "-";
        (*dest)[name + "_max"] = "-";
    }
    if (include_rate) {
        (*dest)[name + "_persec"] = format(aggregated.count / ticks_to_secs(length));
    }

    delete[] stats;
};

/* perfmon_stddev_t */

perfmon_stddev_t::perfmon_stddev_t(std::string name, bool internal)
    : perfmon_t(internal), name(name) { }

perfmon_stddev_t::stats_t *perfmon_stddev_t::get() {
    return &thread_info[get_thread_id()];
}

void *perfmon_stddev_t::begin_stats() {
    return new stats_t[get_num_threads()];
}

void perfmon_stddev_t::visit_stats(void *data) {
    ((stats_t*)data)[get_thread_id()] = *get();
}

void perfmon_stddev_t::end_stats(void *data, perfmon_stats_t *dest) {
    stats_t *stats = (stats_t*) data;

    // See http://en.wikipedia.org/wiki/Standard_deviation#Combining_standard_deviations
    // N{,_i}: datapoints in {total,ith thread}
    // M{,_i}: mean of {total,ith thread}
    // V{,_i}: variance of {total,ith thread}
    // M = (\sum_i N_i M_i) / N
    // V = (\sum_i N_i (V_i + M_i^2)) / N - M^2

    float total_datapoints = 0.0; // becomes N
    float total_means = 0.0;      // becomes \sum_i M_i
    float total_var = 0.0;        // becomes \sum_i N_i (V_i + M_i^2)
    for (int i = 0; i < get_num_threads(); ++i) {
        const stats_t &stat = stats[i];
        float N = stat.datapoints(), M = stat.mean(), V = stat.standard_variance();
        total_datapoints += N;
        total_means += N * M;
        total_var += N * (V + M * M);
    }

    if (total_datapoints) {
        float mean = total_means / total_datapoints;
        float variance = total_var / total_datapoints - mean * mean;
        float deviation = sqrt(variance);
        (*dest)[name + "_mean"] = format(mean);
        (*dest)[name + "_var"] = format(variance);
        (*dest)[name + "_dev"] = format(deviation);
    } else {
        // No stats
        (*dest)[name + "_mean"] = "-";
        (*dest)[name + "_var"] = "-";
        (*dest)[name + "_dev"] = "-";
    }

    delete[] stats;
}

void perfmon_stddev_t::record(float value) { get()->add(value); }

perfmon_stddev_t::stats_t::stats_t() :
#ifndef NAN
#error "Implementation doesn't support NANs"
#endif
    N(0), M(NAN), Q(NAN) { }

void perfmon_stddev_t::stats_t::add(float value) {
    // See http://www.cs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
    size_t k = N += 1;          // NB. the paper indexes from 1
    if (k != 1) {
        // Q_k = Q_{k-1} + ((k-1) (x_k - M_{k-1})^2) / k
        // M_k = M_{k-1} + (x_k - M_{k-1}) / k
        float temp = value - M;
        M += temp / k;
        Q += ((k - 1) * temp * temp) / k;
    } else {
        M = value;
        Q = 0.0;
    }
}

size_t perfmon_stddev_t::stats_t::datapoints() const { return N; }
float perfmon_stddev_t::stats_t::mean() const { return M; }
float perfmon_stddev_t::stats_t::standard_variance() const { return Q / N; }
float perfmon_stddev_t::stats_t::standard_deviation() const { return sqrt(standard_variance()); }

/* perfmon_rate_monitor_t */

perfmon_rate_monitor_t::perfmon_rate_monitor_t(std::string name, ticks_t length, bool internal)
    : perfmon_t(internal), name(name), length(length)
{
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_info[i].current_interval = get_ticks() / length;
    }
}

void perfmon_rate_monitor_t::update(ticks_t now) {
    int interval = now / length;
    thread_info_t *thread = &thread_info[get_thread_id()];

    if (thread->current_interval == interval) {
        /* We're up to date; nothing to do */
    } else if (thread->current_interval + 1 == interval) {
        /* We're one step behind */
        thread->last_count = thread->current_count;
        thread->current_count = 0;
        thread->current_interval++;
    } else {
        /* We're more than one step behind */
        thread->last_count = thread->current_count = 0;
        thread->current_interval = interval;
    }
}

void perfmon_rate_monitor_t::record(double count) {
    ticks_t now = get_ticks();
    update(now);
    thread_info_t *thread = &thread_info[get_thread_id()];
    thread->current_count += count;
}

void *perfmon_rate_monitor_t::begin_stats() {
    return new double[get_num_threads()];
}

void perfmon_rate_monitor_t::visit_stats(void *data) {
    update(get_ticks());
    /* Return last_count instead of current_stats so that we can give a complete interval's
    worth of stats. We might be halfway through an interval, in which case current_count will
    only have half an interval worth. */
    ((double *)data)[get_thread_id()] = thread_info[get_thread_id()].last_count;
}

void perfmon_rate_monitor_t::end_stats(void *data, perfmon_stats_t *dest) {
    double *stats = (double *)data;
    double total = 0;
    for (int i = 0; i < get_num_threads(); i++) {
        total += stats[i];
    }

    (*dest)[name] = format(total / ticks_to_secs(length));

    delete[] stats;
};

/* perfmon_function_t */

perfmon_function_t::internal_function_t::internal_function_t(perfmon_function_t *p)
    : parent(p)
{
    parent->funs[get_thread_id()].push_back(this);
}

perfmon_function_t::internal_function_t::~internal_function_t() {
    parent->funs[get_thread_id()].remove(this);
}

void *perfmon_function_t::begin_stats() {
    return reinterpret_cast<void*>(new std::string());
}

void perfmon_function_t::visit_stats(void *data) {
    std::string *string = reinterpret_cast<std::string*>(data);
    for (internal_function_t *f = funs[get_thread_id()].head(); f; f = funs[get_thread_id()].next(f)) {
        if (string->size() > 0) (*string) += ", ";
        (*string) += f->compute_stat();
    }
}

void perfmon_function_t::end_stats(void *data, perfmon_stats_t *dest) {
    std::string *string = reinterpret_cast<std::string*>(data);
    if (string->size()) (*dest)[name] = *string;
    else (*dest)[name] = "N/A";
    delete string;
}
