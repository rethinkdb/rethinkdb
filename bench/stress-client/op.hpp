#ifndef __OP_HPP__
#define __OP_HPP__

#include "utils.hpp"

struct query_stats_t {

    int queries;
    ticks_t worst_latency;
    reservoir_sample_t<ticks_t> latency_samples;

    query_stats_t() : queries(0), worst_latency(0) { }

    void push(ticks_t latency, bool enable_latency_samples, int batch_count) {
        queries += batch_count;
        worst_latency = std::max(worst_latency, latency);
        if (enable_latency_samples) {
            for (int i = 0; i < batch_count; i++) latency_samples.push(latency);
        }
    }

    void aggregate(const query_stats_t &other) {
        queries += other.queries;
        worst_latency = std::max(worst_latency, other.worst_latency);
        latency_samples += other.latency_samples;
    }
};

struct op_t {

    op_t() : enable_latency_samples(true) { }
    virtual ~op_t() { }

    void push_stats(float latency, int count) {
        stats_spinlock.lock();
        stats.push(latency, enable_latency_samples, count);
        stats_spinlock.unlock();
    }

    spinlock_t stats_spinlock;
    query_stats_t stats;
    bool enable_latency_samples;

    virtual void run() = 0;
};

#endif /* __OP_HPP__ */
