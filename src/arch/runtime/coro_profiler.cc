// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/coro_profiler.hpp"

#include <string>
#include <vector>
#include <sstream>

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/runtime.hpp"
#include "backtrace.hpp"
#include "thread_stack_pcs.hpp"
#include "containers/scoped.hpp"
#include "logger.hpp"


#ifdef ENABLE_CORO_PROFILER

// In order to not waste the first couple entries of our back traces on constant
// entries inside of `rethinkdb_backtrace()`, we strip those frames off.
// `NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE` is the number of frames that must be removed
// to hide the call to `rethinkdb_backtrace()` from the backtrace.
// This depends on the implementation of `rethinkdb_backtrace()`.
// Hmm, maybe it should be moved to there?
#define NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE   1

coro_profiler_t &coro_profiler_t::get_global_profiler() {
    // Singleton implementation after Scott Meyers.
    // Since C++11, this is even thread safe according to here:
    // http://stackoverflow.com/questions/1661529/is-meyers-implementation-of-singleton-pattern-thread-safe?lq=1
    static coro_profiler_t profiler;
    return profiler;
}

coro_profiler_t::coro_profiler_t() {
    logINF("Coro profiler activated.");
    reql_output_file.open("coro_profiler_out.py");
    if (reql_output_file.is_open()) {
        logINF("Writing output to reql_output_file");
        write_reql_header();
    }
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
    per_thread_samples_t &thread_samples = per_thread_samples[get_thread_id().threadnum].value;
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
        
        // Figure out from where we were called.
        // We strip ourselves from the backtrace, hence the +1 in the argument to
        // `get_current_execution_point().`
        const coro_execution_point_key_t execution_point =
            get_current_execution_point(levels_to_strip_from_backtrace + 1);
        
        // Record the sample
        per_execution_point_samples_t &execution_point_samples = thread_samples.per_execution_point_samples[execution_point];
        ++execution_point_samples.num_samples_total;
        ticks_t ticks_since_previous = 0;
        if (coro_mixin.last_sample_at != 0) {
            rassert(coro_mixin.last_sample_at <= ticks_on_entry);
            ticks_since_previous = ticks_on_entry - coro_mixin.last_sample_at;
        }
        rassert(coro_mixin.last_resumed_at <= ticks_on_entry);
        rassert(coro_mixin.last_resumed_at > 0);
        ticks_t ticks_since_resume = ticks_on_entry - coro_mixin.last_resumed_at;
        execution_point_samples.samples.push_back(coro_sample_t(ticks_since_resume, ticks_since_previous));
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

void coro_profiler_t::record_coro_yield(size_t levels_to_strip_from_backtrace) {
    rassert(coro_t::self());
    
    record_sample(1 + levels_to_strip_from_backtrace);
}

coro_profiler_t::coro_execution_point_key_t coro_profiler_t::get_current_execution_point(size_t levels_to_strip_from_backtrace) {
    // Generate call trace
    // We strip ourselves, and the frames that are inside `rethinkdb_backtrace()`.
    levels_to_strip_from_backtrace += 1 + NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE;
    const size_t max_frames = CORO_PROFILER_CALLTREE_DEPTH + levels_to_strip_from_backtrace;
    void **stack_frames = new void*[max_frames];
    size_t backtrace_size = rethinkdb_backtrace(stack_frames, max_frames);
    small_trace_t trace;
    for (size_t i = 0; i < CORO_PROFILER_CALLTREE_DEPTH; ++i) {
        if (i + levels_to_strip_from_backtrace < backtrace_size) {
            trace[i] = stack_frames[i + levels_to_strip_from_backtrace];
        } else {
            trace[i] = NULL;
        }
    }
    delete[] stack_frames;

    // Record the sample
#ifndef NDEBUG
    return coro_execution_point_key_t(coro_t::self()->get_coroutine_type(), trace);
#else
    return coro_execution_point_key_t("?", trace);
#endif
}

void coro_profiler_t::generate_report() {
    std::map<coro_execution_point_key_t, per_execution_point_collected_report_t> execution_point_reports;
    
    // We assume that the global report_interval_spinlock has already been locked by our callee.
    {
        // Proceed to locking all thread sample structures
        std::vector<scoped_ptr_t<spinlock_acq_t> > thread_locks;
        for (auto thread_samples = per_thread_samples.begin(); thread_samples != per_thread_samples.end(); ++thread_samples) {
            thread_locks.push_back(scoped_ptr_t<spinlock_acq_t>(new spinlock_acq_t(&thread_samples->value.spinlock)));
        }

        // Reset report ticks
        const ticks_t ticks = get_ticks();
        ticks_at_last_report = ticks;
        for (auto thread_samples = per_thread_samples.begin(); thread_samples != per_thread_samples.end(); ++thread_samples) {
            thread_samples->value.ticks_at_last_report = ticks;
        }

        // Collect data for each trace over all threads
        for (auto thread_samples = per_thread_samples.begin(); thread_samples != per_thread_samples.end(); ++thread_samples) {
            for (auto execution_point_samples = thread_samples->value.per_execution_point_samples.begin(); execution_point_samples != thread_samples->value.per_execution_point_samples.end(); ++execution_point_samples) {
                for (auto sample = execution_point_samples->second.samples.begin(); sample != execution_point_samples->second.samples.end(); ++sample) {
                    execution_point_reports[execution_point_samples->first].collect(*sample);
                }

                // Reset samples
                execution_point_samples->second.samples.clear();
            }
        }
        
        // Release per-thread locks
    }
    
    if (reql_output_file.is_open()) {
        print_to_reql(execution_point_reports);
    } else {
        print_to_console(execution_point_reports);
    }
}

void coro_profiler_t::print_to_console(
    const std::map<coro_execution_point_key_t, per_execution_point_collected_report_t> &execution_point_reports) {
    
    printf("\nNUM\tTIME_PREV\tTIME_RES\n");
    for (auto report = execution_point_reports.begin(); report != execution_point_reports.end(); ++report) {
        std::string formatted_execution_point = format_execution_point(report->first);
        printf("%i\t%f\t%f\n%s",
               report->second.num_samples,
               report->second.get_avg_time_since_previous(),
               report->second.get_avg_time_since_resume(),
               formatted_execution_point.c_str());
    } 
}

void coro_profiler_t::print_to_reql(
    const std::map<coro_execution_point_key_t, per_execution_point_collected_report_t> &execution_point_reports) {
    
    const double time = ticks_to_secs(get_ticks());
    
    for (auto report = execution_point_reports.begin(); report != execution_point_reports.end(); ++report) {
        std::string trace_array_str = "[";
        for (size_t i = 0; i < CORO_PROFILER_CALLTREE_DEPTH; ++i) {
            if (report->first.second[i] == NULL) {
                break;
            }
            if (i > 0) {
                trace_array_str += ", ";
            }
            trace_array_str += "'" + get_frame_description(report->first.second[i]) + "'";
        }
        trace_array_str += "]";
        
        reql_output_file << "print t.insert({" << std::endl;
        reql_output_file << "\t\t'time': " << time << "," << std::endl;
        reql_output_file << "\t\t'coro_type': '" << report->first.first << "'," << std::endl;
        reql_output_file << "\t\t'trace': " << trace_array_str << "," << std::endl;
        reql_output_file << "\t\t'num_samples': " << report->second.num_samples << "," << std::endl;
        reql_output_file << "\t\t'since_previous_avg': " << report->second.get_avg_time_since_previous() << "," << std::endl;
        reql_output_file << "\t\t'since_resume_avg': " << report->second.get_avg_time_since_resume() << "" << std::endl;
        reql_output_file << "\t}).run(conn, durability='soft')" << std::endl;
    }
}

void coro_profiler_t::write_reql_header() {
    reql_output_file << "#!/usr/bin/env python" << std::endl;
    reql_output_file << std::endl;
    reql_output_file << "import rethinkdb as r" << std::endl;
    reql_output_file << "conn = r.connect() # Modify this as needed" << std::endl;
    reql_output_file << "t = r.table(\"coro_prof\") # Modify this as needed" << std::endl;
    reql_output_file << std::endl;
}

std::string coro_profiler_t::format_execution_point(const coro_execution_point_key_t &execution_point) {
    std::stringstream format_stream;
    // This output could be nicer...
    format_stream << "\t: " << execution_point.first << "\n";
    for (auto frame = execution_point.second.begin(); frame != execution_point.second.end(); ++frame) {
        if (*frame == NULL) break;
        format_stream << "\t| " << get_frame_description(*frame) << "\n";
    }
    return format_stream.str();
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
        demangled_name = frame.get_demangled_name();
    } catch (demangle_failed_exc_t &e) {
        demangled_name = "?";
    }
    description_stream << frame.get_addr() << "\t" << demangled_name;
    
    return frame_description_cache.insert(std::pair<void *, std::string>(addr, description_stream.str())).first->second;
}

#endif /* ENABLE_CORO_PROFILER */
