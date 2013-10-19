// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/coro_profiler.hpp"

#include <string>
#include <vector>
#include <sstream>

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/runtime.hpp"
#include "backtrace.hpp"
#include "containers/scoped.hpp"
#include "logger.hpp"


#ifdef ENABLE_CORO_PROFILER

// TODO: Document better
// This depends on the implementation of backtrace_t. Maybe it should be moved to there?
#define NUM_FRAMES_INSIDE_BACKTRACE_T   2

coro_profiler_t &coro_profiler_t::get_global_profiler() {
    // Singleton implementation after Scott Meyers.
    // Since C++11, this is even thread safe according to here:
    // http://stackoverflow.com/questions/1661529/is-meyers-implementation-of-singleton-pattern-thread-safe?lq=1
    static coro_profiler_t profiler;
    return profiler;
}

coro_profiler_t::coro_profiler_t() {
    logINF("Coro profiler activated.");
}

coro_profiler_t::~coro_profiler_t() {
    // Generate a final report:
    const spinlock_acq_t report_interval_lock(&report_interval_spinlock);
    generate_report();
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
        small_trace_t trace;
        // We strip ourselves, and the frames that are inside of the backtrace_t constructor.
        levels_to_strip_from_backtrace += 1 + NUM_FRAMES_INSIDE_BACKTRACE_T;
        for (size_t i = 0; i < CORO_PROFILER_CALLTREE_DEPTH; ++i) {
            if (i + levels_to_strip_from_backtrace < full_backtrace.get_num_frames()) {
                trace[i] = full_backtrace.get_frame(i + levels_to_strip_from_backtrace).get_addr();
            } else {
                trace[i] = NULL;
            }
        }
        
        // Record the sample
        per_trace_samples_t &trace_samples = thread_samples.per_trace_samples[trace];
        ++trace_samples.num_samples_total;
        ticks_t ticks_since_previous = 0;
        if (coro_mixin.last_sample_at != 0) {
            rassert(coro_mixin.last_sample_at <= ticks_on_entry);
            ticks_since_previous = ticks_on_entry - coro_mixin.last_sample_at;
        }
        rassert(coro_mixin.last_resumed_at <= ticks_on_entry);
        rassert(coro_mixin.last_resumed_at > 0);
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
    coro_mixin.last_sample_at = ticks;
    coro_mixin.last_resumed_at = ticks;
}

void coro_profiler_t::record_coro_yield() {
    rassert(coro_t::self());
    
    record_sample(1);
}

// TODO!
#include <iostream>
void coro_profiler_t::generate_report() {
    // We assume that the global report_interval_spinlock has already been locked by our callee.
    // Proceed to locking all thread sample structures
    std::vector<scoped_ptr_t<spinlock_acq_t> > thread_locks;
    for (auto thread_samples = per_thread_samples.begin(); thread_samples != per_thread_samples.end(); ++thread_samples) {
        thread_locks.push_back(scoped_ptr_t<spinlock_acq_t>(new spinlock_acq_t(&thread_samples->spinlock)));
    }
    
    // Reset report ticks
    const ticks_t ticks = get_ticks();
    ticks_at_last_report = ticks;
    for (auto thread_samples = per_thread_samples.begin(); thread_samples != per_thread_samples.end(); ++thread_samples) {
        thread_samples->ticks_at_last_report = ticks;
    }
    
    // Generate the actual report
    struct per_trace_collected_report_t {
        per_trace_collected_report_t() :
            num_samples(0),
            total_time_since_previous(0.0),
            total_time_since_resume(0.0) {
        }
        double get_avg_time_since_previous() const {
            return total_time_since_previous / std::max(1.0, static_cast<double>(num_samples));
        }
        double get_avg_time_since_resume() const {
            return total_time_since_resume / std::max(1.0, static_cast<double>(num_samples));
        }
        void collect(const trace_sample_t &sample) {
            total_time_since_previous += ticks_to_secs(sample.ticks_since_previous);
            total_time_since_resume += ticks_to_secs(sample.ticks_since_resume);
            ++num_samples;
        }

        int num_samples;
        // TODO: Also report total count
        double total_time_since_previous;
        double total_time_since_resume;
    };
    
    // Collect data for each trace over all threads
    std::map<small_trace_t, per_trace_collected_report_t> trace_reports;
    for (auto thread_samples = per_thread_samples.begin(); thread_samples != per_thread_samples.end(); ++thread_samples) {
        for (auto trace_samples = thread_samples->per_trace_samples.begin(); trace_samples != thread_samples->per_trace_samples.end(); ++trace_samples) {
            for (auto sample = trace_samples->second.samples.begin(); sample != trace_samples->second.samples.end(); ++sample) {
                trace_reports[trace_samples->first].collect(*sample);
            }
            
            // Reset samples
            trace_samples->second.samples.clear();
        }
    }
    
    // TODO: Sort the output
    // TODO: Make this look nicer
    printf("\nNUM\tTIME_PREV\tTIME_RES\n");
    for (auto report = trace_reports.begin(); report != trace_reports.end(); ++report) {
        // TODO: Do we use something else instead of printf?
        std::string formatted_trace = format_trace(report->first);
        printf("%i\t%f\t%f\n%s",
               report->second.num_samples,
               report->second.get_avg_time_since_previous(),
               report->second.get_avg_time_since_resume(),
               formatted_trace.c_str());
    }
}

std::string coro_profiler_t::format_trace(const small_trace_t &trace) {
    std::stringstream trace_stream;
    // TODO: Make this look nicer
    for (auto frame = trace.begin(); frame != trace.end(); ++frame) {
        if (*frame == NULL) break;
        trace_stream << "| " << get_frame_description(*frame) << "\n";
    }
    return trace_stream.str();
}

const std::string &coro_profiler_t::get_frame_description(void *addr) {
    auto cache_it = frame_description_cache.find(addr);
    if (cache_it != frame_description_cache.end()) {
        return cache_it->second;
    }
    
    backtrace_frame_t frame(addr);
    std::stringstream description_stream;
    // TODO: Would be nice if we could also use addr2line here
    std::string demangled_name;
    try {
        // TODO: Just for testing, use the mangled name. It's much faster.
        //demangled_name = frame.get_demangled_name();
        demangled_name = frame.get_name();
    } catch (demangle_failed_exc_t &e) {
        demangled_name = "?";
    }
    description_stream << frame.get_addr() << "\t" << demangled_name;
    
    return frame_description_cache.insert(std::pair<void *, std::string>(addr, description_stream.str())).first->second;
}

#endif /* ENABLE_CORO_PROFILER */
