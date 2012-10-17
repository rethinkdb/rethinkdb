#include "perfmon/perfmon.hpp"

#include <stdarg.h>
#include <math.h>
#include <map>

#include "utils.hpp"
#include <boost/bind.hpp>

#include "concurrency/pmap.hpp"
#include "arch/arch.hpp"

static const char * stat_avg = "avg";
static const char * stat_min = "min";
static const char * stat_max = "max";
static const char * stat_per_sec = "per_sec";
static const char * stat_count = "count";
static const char * stat_mean = "mean";
static const char * stat_std_dev = "std_dev";
static const char * no_value = "-";


bool global_full_perfmon = false;

/* perfmon_counter_t */

perfmon_counter_t::perfmon_counter_t()
    : perfmon_perthread_t<cache_line_padded_t<int64_t>, int64_t>(),
      thread_data(new padded_int64_t[MAX_THREADS])
{
    for (int i = 0; i < MAX_THREADS; i++) thread_data[i].value = 0;
}

perfmon_counter_t::~perfmon_counter_t() {
    delete[] thread_data;
}

int64_t &perfmon_counter_t::get() {
    rassert(get_thread_id() >= 0);
    return thread_data[get_thread_id()].value;
}

void perfmon_counter_t::get_thread_stat(padded_int64_t *stat) {
    stat->value = get();
}

int64_t perfmon_counter_t::combine_stats(padded_int64_t *data) {
    int64_t value = 0;
    for (int i = 0; i < get_num_threads(); i++) value += data[i].value;
    return value;
}

perfmon_result_t *perfmon_counter_t::output_stat(const int64_t &stat) {
    return new perfmon_result_t(strprintf("%" PRIi64, stat));
}

/* perfmon_sampler_t */

perfmon_sampler_t::perfmon_sampler_t(ticks_t _length, bool _include_rate)
    : perfmon_perthread_t<stats_t>(), thread_data(new thread_info_t[MAX_THREADS]), length(_length), include_rate(_include_rate)
{
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_data[i].current_interval = get_ticks() / length;
    }
}

perfmon_sampler_t::~perfmon_sampler_t() {
    delete[] thread_data;
}

void perfmon_sampler_t::update(ticks_t now) {
    int interval = now / length;
    rassert(get_thread_id() >= 0);
    thread_info_t *thread = &thread_data[get_thread_id()];

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

void perfmon_sampler_t::record(double v) {
    ticks_t now = get_ticks();
    update(now);
    rassert(get_thread_id() >= 0);
    thread_info_t *thread = &thread_data[get_thread_id()];
    thread->current_stats.record(v);
}

void perfmon_sampler_t::get_thread_stat(stats_t *stat) {
    update(get_ticks());
    /* Return last_stats instead of current_stats so that we can give a complete interval's
    worth of stats. We might be halfway through an interval, in which case current_stats will
    only have half an interval worth. */
    rassert(get_thread_id() >= 0);
    *stat = thread_data[get_thread_id()].last_stats;
}

perfmon_sampler_t::stats_t perfmon_sampler_t::combine_stats(stats_t *stats) {
    stats_t aggregated;
    for (int i = 0; i < get_num_threads(); i++) {
        aggregated.aggregate(stats[i]);
    }
    return aggregated;
}

perfmon_result_t *perfmon_sampler_t::output_stat(const stats_t &aggregated) {
    perfmon_result_t *stat;
    perfmon_result_t::alloc_map_result(&stat);

    if (aggregated.count > 0) {
        stat->insert(stat_avg, new perfmon_result_t(strprintf("%.8f", aggregated.sum / aggregated.count)));
        stat->insert(stat_min, new perfmon_result_t(strprintf("%.8f", aggregated.min)));
        stat->insert(stat_max, new perfmon_result_t(strprintf("%.8f", aggregated.max)));
    } else {
        stat->insert(stat_avg, new perfmon_result_t(no_value));
        stat->insert(stat_min, new perfmon_result_t(no_value));
        stat->insert(stat_max, new perfmon_result_t(no_value));
    }
    if (include_rate) {
        stat->insert(stat_per_sec, new perfmon_result_t(strprintf("%.8f", aggregated.count / ticks_to_secs(length))));
    }
    return stat;
}

/* perfmon_stddev_t */

stddev_t::stddev_t()
#ifndef NAN
#error "Implementation doesn't support NANs"
#endif
    : N(0), M(NAN), Q(NAN) { }

stddev_t::stddev_t(size_t n, double mean, double variance)
    : N(n), M(mean), Q(variance * n) {
    if (N == 0)
        rassert(isnan(M) && isnan(Q));
}

void stddev_t::add(double value) {
    // See http://www.cs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
    size_t k = N += 1;          // NB. the paper indexes from 1
    if (k != 1) {
        // Q_k = Q_{k-1} + ((k-1) (x_k - M_{k-1})^2) / k
        // M_k = M_{k-1} + (x_k - M_{k-1}) / k
        double temp = value - M;
        M += temp / k;
        Q += ((k - 1) * temp * temp) / k;
    } else {
        M = value;
        Q = 0.0;
    }
}

size_t stddev_t::datapoints() const { return N; }
double stddev_t::mean() const { return M; }
double stddev_t::standard_variance() const { return Q / N; }
double stddev_t::standard_deviation() const { return sqrt(standard_variance()); }

stddev_t stddev_t::combine(size_t nelts, stddev_t *data) {
    // See http://en.wikipedia.org/wiki/Standard_deviation#Combining_standard_deviations
    // N{,_i}: datapoints in {total,ith thread}
    // M{,_i}: mean of {total,ith thread}
    // V{,_i}: variance of {total,ith thread}
    // M = (\sum_i N_i M_i) / N
    // V = (\sum_i N_i (V_i + M_i^2)) / N - M^2

    size_t total_datapoints = 0; // becomes N
    double total_means = 0.0;     // becomes \sum_i N_i M_i
    double total_var = 0.0;       // becomes \sum_i N_i (V_i + M_i^2)
    for (size_t i = 0; i < nelts; ++i) {
        const stddev_t &stat = data[i];
        size_t N = stat.datapoints();
        if (!N) continue;
        double M = stat.mean(), V = stat.standard_variance();
        total_datapoints += N;
        total_means += N * M;
        total_var += N * (V + M * M);
    }

    if (total_datapoints) {
        double mean = total_means / total_datapoints;
        return stddev_t(total_datapoints, mean, total_var / total_datapoints - mean * mean);
    }
    return stddev_t();
}

perfmon_stddev_t::perfmon_stddev_t() : perfmon_perthread_t<stddev_t>() { }

void perfmon_stddev_t::get_thread_stat(stddev_t *stat) {
    rassert(get_thread_id() >= 0);
    *stat = thread_data[get_thread_id()];
}

stddev_t perfmon_stddev_t::combine_stats(stddev_t *stats) {
    return stddev_t::combine(get_num_threads(), stats);
}

perfmon_result_t *perfmon_stddev_t::output_stat(const stddev_t &stat_data) {
    perfmon_result_t *stat;
    perfmon_result_t::alloc_map_result(&stat);

    stat->insert(stat_count, new perfmon_result_t(strprintf("%zu", stat_data.datapoints())));
    if (stat_data.datapoints()) {
        stat->insert(stat_mean, new perfmon_result_t(strprintf("%.8f", stat_data.mean())));
        stat->insert(stat_std_dev, new perfmon_result_t(strprintf("%.8f", stat_data.standard_deviation())));
    } else {
        // No stats
        stat->insert(stat_mean, new perfmon_result_t(no_value));
        stat->insert(stat_std_dev, new perfmon_result_t(no_value));
    }
    return stat;
}

void perfmon_stddev_t::record(double value) {
    rassert(get_thread_id() >= 0);
    thread_data[get_thread_id()].add(value);
}

/* perfmon_rate_monitor_t */

perfmon_rate_monitor_t::perfmon_rate_monitor_t(ticks_t _length)
    : perfmon_perthread_t<double>(), length(_length)
{
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_data[i].current_interval = get_ticks() / length;
    }
}

void perfmon_rate_monitor_t::update(ticks_t now) {
    int interval = now / length;
    rassert(get_thread_id() >= 0);
    thread_info_t &thread = thread_data[get_thread_id()];

    if (thread.current_interval == interval) {
        /* We're up to date; nothing to do */
    } else if (thread.current_interval + 1 == interval) {
        /* We're one step behind */
        thread.last_count = thread.current_count;
        thread.current_count = 0;
        thread.current_interval++;
    } else {
        /* We're more than one step behind */
        thread.last_count = thread.current_count = 0;
        thread.current_interval = interval;
    }
}

void perfmon_rate_monitor_t::record(double count) {
    ticks_t now = get_ticks();
    update(now);
    rassert(get_thread_id() >= 0);
    thread_info_t &thread = thread_data[get_thread_id()];
    thread.current_count += count;
}

void perfmon_rate_monitor_t::get_thread_stat(double *stat) {
    update(get_ticks());
    /* Return last_count instead of current_stats so that we can give a complete interval's
    worth of stats. We might be halfway through an interval, in which case current_count will
    only have half an interval worth. */
    rassert(get_thread_id() >= 0);
    *stat = thread_data[get_thread_id()].last_count;
}

double perfmon_rate_monitor_t::combine_stats(double *stats) {
    double total = 0;
    for (int i = 0; i < get_num_threads(); i++) total += stats[i];
    return total;
}

perfmon_result_t *perfmon_rate_monitor_t::output_stat(const double &stat) {
    return new perfmon_result_t(strprintf("%.8f", stat / ticks_to_secs(length)));
}

perfmon_duration_sampler_t::perfmon_duration_sampler_t(ticks_t length, bool _ignore_global_full_perfmon)
    : stat(), active(), total(), recent(length, true),
      active_membership(&stat, &active, "active_count"),
      total_membership(&stat, &total, "total"),
      recent_membership(&stat, &recent, "recent_duration"),
      ignore_global_full_perfmon(_ignore_global_full_perfmon)
{ }

void perfmon_duration_sampler_t::begin(ticks_t *v) {
    ++active;
    ++total;
    if (global_full_perfmon || ignore_global_full_perfmon) {
        *v = get_ticks();
    } else {
        *v = 0;
    }
}

void perfmon_duration_sampler_t::end(ticks_t *v) {
    --active;
    if (*v != 0) {
        recent.record(ticks_to_secs(get_ticks() - *v));
    }
}

void *perfmon_duration_sampler_t::begin_stats() {
    return stat.begin_stats();
}

void perfmon_duration_sampler_t::visit_stats(void *data) {
    stat.visit_stats(data);
}

perfmon_result_t *perfmon_duration_sampler_t::end_stats(void *data) {
    return stat.end_stats(data);
}

std::string perfmon_duration_sampler_t::call(UNUSED int argc, UNUSED char **argv) {
    ignore_global_full_perfmon = !ignore_global_full_perfmon;
    if (ignore_global_full_perfmon) {
        return "Enabled\n";
    } else {
        return "Disabled\n";
    }
}

