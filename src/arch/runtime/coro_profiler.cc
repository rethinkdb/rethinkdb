// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/coro_profiler.hpp"

#include <string>

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/runtime.hpp"
#include "backtrace.hpp"


#ifdef ENABLE_CORO_PROFILER

coro_profiler_t &coro_profiler_t::get_global_profiler() {
    // Singleton implementation after Scott Meyers.
    // Since C++11, this is even thread safe according to here:
    // http://stackoverflow.com/questions/1661529/is-meyers-implementation-of-singleton-pattern-thread-safe?lq=1
    static coro_profiler_t profiler;
    return profiler;
}

void coro_profiler_t::record_sample(size_t levels_to_strip_from_backtrace) {
    if (coro_t::self() == NULL) return;
    
    const ticks_t ticks_on_entry = get_ticks();
    
    coro_profiler_mixin_t &coro_mixin = static_cast<coro_profiler_mixin_t&>(*coro_t::self());
    bool might_have_to_generate_report = false;
    per_thread_samples_t &thread_samples = per_thread_samples[get_thread_id().threadnum];
    {        
        const spinlock_acq_t thread_lock(&thread_samples.spinlock);
        
        // See if we might have to generate a report
        if (thread_samples.ticks_at_last_report + CORO_PROFILER_REPORTING_INTERVAL <= ticks_on_entry) {
            // There is a chance that we have to generate a report (unless another
            // thread is already at it). Let's first release the lock on thread_samples
            // before we actually check for that though. That way we can be sure
            // not to dead-lock.
            might_have_to_generate_report = true;
        }
        
        // Generate call trace
        backtrace_t full_backtrace;
        std::array<void *, CORO_PROFILER_CALLTREE_DEPTH> trace;
        // TODO! strip a few additional levels. At least ourselves.
        levels_to_strip_from_backtrace += 1 + 0; // We strip ourselves, and TODO
        for (size_t i = 0; i < CORO_PROFILER_CALLTREE_DEPTH; ++i) {
            if (i + levels_to_strip_from_backtrace < full_backtrace.get_num_frames()) {
                trace[i] = full_backtrace.get_frame(i + levels_to_strip_from_backtrace).get_addr();
            } else {
                trace[i] = NULL;
            }
        }
        
        // Record the sample
        per_trace_samples_t &trace_samples = thread_samples.per_trace_samples[trace];
        ++trace_samples.num_samples_during_interval;
        ++trace_samples.num_samples_total;
        ticks_t ticks_since_previous = 0;
        if (coro_mixin.last_sample_at != 0) {
            rassert(coro_mixin.last_sample_at <= ticks_on_entry);
            ticks_since_previous = ticks_on_entry - coro_mixin.last_sample_at;
        }
        rassert(coro_mixin.last_resumed_at <= ticks_on_entry);
        ticks_t ticks_since_resume = ticks_on_entry - coro_mixin.last_resumed_at;
        trace_samples.samples.push_back(trace_sample_t(ticks_since_resume, ticks_since_previous));
        coro_mixin.last_sample_at = ticks_on_entry;
    }
    
    if (might_have_to_generate_report) {
        const spinlock_acq_t report_interval_lock(&report_interval_spinlock);
        if (ticks_at_last_report + CORO_PROFILER_REPORTING_INTERVAL <= ticks_on_entry) {
            generate_report();
        }
    }
    
    /* 
     * Ensign:  Captain, we are no longer in warp.
     * Captain: What happened?
     * Ensign:  The profiler is stealing our time and makes us appear slower than we actually are!
     * Captain: Can we compensate?
     * Ensign:  Positive.
     * Captain: Do it!
     * Ensign:  I'm reconfiguring the shield er yield timings. That should block its interference.
     */
    const ticks_t ticks_on_exit = get_ticks();
    rassert(ticks_on_exit >= ticks_on_entry);
    const ticks_t clock_scew = ticks_on_exit - ticks_on_entry;
    coro_mixin.last_resumed_at += clock_scew;
    coro_mixin.last_sample_at += clock_scew;
}

void coro_profiler_t::record_coro_resume() {
    rassert(coro_t::self());
    
    const ticks_t ticks = get_ticks();
    
    coro_profiler_mixin_t &coro_mixin = static_cast<coro_profiler_mixin_t&>(*coro_t::self());
    coro_mixin.last_resumed_at = ticks;
}

void coro_profiler_t::record_coro_yield() {
    rassert(coro_t::self());
    
    record_sample(1);
}

void coro_profiler_t::generate_report() {
    // TODO!
}

#endif /* ENABLE_CORO_PROFILER */
