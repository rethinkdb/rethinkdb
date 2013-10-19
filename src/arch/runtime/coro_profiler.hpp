// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_CORO_PROFILER_HPP_
#define	ARCH_RUNTIME_CORO_PROFILER_HPP_


#ifdef ENABLE_CORO_PROFILER

#include <array>
#include <map>

#include "utils.hpp"
#include "arch/spinlock.hpp"
#include "config/args.hpp"

#define CORO_PROFILER_CALLTREE_DEPTH            3
#define CORO_PROFILER_REPORTING_INTERVAL        (secs_to_ticks(1) * 10)


/* TODO!
 * Document how to use this
 * 
 */

class coro_profiler_t {
public:
    // Should you ever want to make this a true singleton, just make the
    // constructor private.
    coro_profiler_t();
    ~coro_profiler_t();
    
    static coro_profiler_t &get_global_profiler();
    
    void record_sample(size_t levels_to_strip_from_backtrace = 0);
    
    // coroutine execution is resumed
    void record_coro_resume();
    // coroutine execution yields
    void record_coro_yield();
    
private:
    typedef std::array<void *, CORO_PROFILER_CALLTREE_DEPTH> small_trace_t;
    struct trace_sample_t {
        trace_sample_t(ticks_t _ticks_since_resume, ticks_t _ticks_since_previous) :
            ticks_since_resume(_ticks_since_resume), ticks_since_previous(_ticks_since_previous) { }
        ticks_t ticks_since_resume;
        ticks_t ticks_since_previous;
    };
    struct per_trace_samples_t {
        per_trace_samples_t() : num_samples_total(0) { }
        int num_samples_total;
        std::vector<trace_sample_t> samples;
    };
    struct per_thread_samples_t {
        per_thread_samples_t() : ticks_at_last_report(get_ticks()) { }
        std::map<small_trace_t, per_trace_samples_t> per_trace_samples;
        spinlock_t spinlock;
        ticks_t ticks_at_last_report; // When one of the threads goes over CORO_PROFILER_REPORTING_INTERVAL, it calls report which resets this entry on all threads.
    };
    
    void generate_report();
    std::string format_trace(const small_trace_t &trace);
    const std::string &get_frame_description(void *addr);
    
    // Would be nice if we could use one_per_thread here. However
    // that makes the construction order tricky.
    // TODO: These should be padded so they have different cache lines.
    std::array<per_thread_samples_t, MAX_THREADS> per_thread_samples;
    
    // TODO: Document
    ticks_t ticks_at_last_report;
    /* Locking order is always: 
     * 1. report_interval_spinlock
     * 2. per_thread_samples.spinlock in ascending order of thread num
     * You can safely skip some of the locks in this order.
     * Acquiring locks in different orders can dead-lock.
     */
    spinlock_t report_interval_spinlock;
    
    std::map<void *, std::string> frame_description_cache;
    
    DISABLE_COPYING(coro_profiler_t);
};

// Short-cuts
// TODO: Document
#define PROFILER_RECORD_SAMPLE_STRIP_FRAMES(STRIP_FRAMES) coro_profiler_t::get_global_profiler().record_sample(STRIP_FRAMES);
#define PROFILER_RECORD_SAMPLE coro_profiler_t::get_global_profiler().record_sample();
#define PROFILER_CORO_RESUME coro_profiler_t::get_global_profiler().record_coro_resume();
#define PROFILER_CORO_YIELD coro_profiler_t::get_global_profiler().record_coro_yield();

#else /* ENABLE_CORO_PROFILER */

// Short-cuts (no-ops for disabled coro profiler)
#define PROFILER_RECORD_SAMPLE_STRIP_FRAMES(STRIP_FRAMES) {}
#define PROFILER_RECORD_SAMPLE {}
#define PROFILER_CORO_RESUME {}
#define PROFILER_CORO_YIELD {}

#endif /* not ENABLE_CORO_PROFILER */

#endif	/* ARCH_RUNTIME_CORO_PROFILER_HPP_ */

