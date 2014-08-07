// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "arch/runtime/coro_profiler.hpp"

#ifdef ENABLE_CORO_PROFILER

#include <stdio.h>

#include <string>
#include <vector>
#include <cmath>

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/runtime.hpp"
#include "rethinkdb_backtrace.hpp"
#include "containers/scoped.hpp"
#include "logger.hpp"

coro_profiler_t &coro_profiler_t::get_global_profiler() {
    // Singleton implementation after Scott Meyers.
    // Since C++11, this is even thread safe according to here:
    // http://stackoverflow.com/questions/1661529/is-meyers-implementation-of-singleton-pattern-thread-safe?lq=1
    static coro_profiler_t profiler;
    return profiler;
}

coro_profiler_t::coro_profiler_t() : reql_output_file(nullptr) {
    logINF("Coro profiler activated.");

    const std::string reql_output_filename = "coro_profiler_out.py";
    reql_output_file = fopen(reql_output_filename.c_str(), "w");
    if (reql_output_file != nullptr) {
        logINF("Writing profiler reports to '%s'", reql_output_filename.c_str());
        write_reql_header();
    } else {
        logWRN("Could not open '%s' for writing profiler reports.", reql_output_filename.c_str());
    }
}

coro_profiler_t::~coro_profiler_t() {
    if (reql_output_file != nullptr) {
        fclose(reql_output_file);
    }
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
        per_execution_point_samples_t &execution_point_samples =
            thread_samples.per_execution_point_samples[execution_point];
        ++execution_point_samples.num_samples_total;
        ticks_t ticks_since_previous = 0;
        if (coro_mixin.last_sample_at != 0) {
            rassert(coro_mixin.last_sample_at <= ticks_on_entry);
            ticks_since_previous = ticks_on_entry - coro_mixin.last_sample_at;
        }
        rassert(coro_mixin.last_resumed_at <= ticks_on_entry);
        rassert(coro_mixin.last_resumed_at > 0);
        ticks_t ticks_since_resume = ticks_on_entry - coro_mixin.last_resumed_at;
        execution_point_samples.samples.push_back(coro_sample_t(ticks_since_resume,
                                                                ticks_since_previous,
                                                                coro_t::self()->get_priority()));
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
     * Ensign:  I'm reconfiguring the shield timings er yield timings. That should block its interference.
     */
    const ticks_t ticks_on_exit = get_ticks();
    rassert(ticks_on_exit >= ticks_on_entry);
    const ticks_t clock_skew = ticks_on_exit - ticks_on_entry;
    coro_mixin.last_resumed_at += clock_skew;
    coro_mixin.last_sample_at += clock_skew;
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

coro_profiler_t::coro_execution_point_key_t coro_profiler_t::get_current_execution_point(
    size_t levels_to_strip_from_backtrace) {

    // Generate call trace
    // We strip ourselves, and the frames that are inside `rethinkdb_backtrace()`.
    levels_to_strip_from_backtrace += 1 + NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE;
    const size_t max_frames = CORO_PROFILER_BACKTRACE_DEPTH + levels_to_strip_from_backtrace;
    void **stack_frames = new void*[max_frames];
    size_t backtrace_size = rethinkdb_backtrace(stack_frames, max_frames);
    size_t remaining_size = max_frames - backtrace_size;
    if (remaining_size > 0) {
        backtrace_size +=
            coro_t::self()->copy_spawn_backtrace(stack_frames + backtrace_size, remaining_size);
    }
    small_trace_t trace;
    for (size_t i = 0; i < CORO_PROFILER_BACKTRACE_DEPTH; ++i) {
        if (i + levels_to_strip_from_backtrace < backtrace_size) {
            trace[i] = stack_frames[i + levels_to_strip_from_backtrace];
        } else {
            trace[i] = NULL;
        }
    }
    delete[] stack_frames;

#ifndef NDEBUG
    return coro_execution_point_key_t(coro_t::self()->get_coroutine_type(), trace);
#else
    return coro_execution_point_key_t("?", trace);
#endif
}

void coro_profiler_t::generate_report() {
    std::map<coro_execution_point_key_t, per_execution_point_collected_report_t> execution_point_reports;

    // We assume that the global report_interval_spinlock has already been locked by our caller.
    {
        // Proceed to locking all thread sample structures
        std::vector<scoped_ptr_t<spinlock_acq_t> > thread_locks;
        for (auto thread_samples = per_thread_samples.begin();
             thread_samples != per_thread_samples.end();
             ++thread_samples) {
            thread_locks.push_back(
                scoped_ptr_t<spinlock_acq_t>(new spinlock_acq_t(&thread_samples->value.spinlock)));
        }

        // Reset report ticks
        const ticks_t ticks = get_ticks();
        ticks_at_last_report = ticks;
        for (auto thread_samples = per_thread_samples.begin();
             thread_samples != per_thread_samples.end();
             ++thread_samples) {
            thread_samples->value.ticks_at_last_report = ticks;
        }

        // Collect data for each trace over all threads
        for (auto thread_samples = per_thread_samples.begin();
             thread_samples != per_thread_samples.end();
             ++thread_samples) {
            for (auto execution_point_samples = thread_samples->value.per_execution_point_samples.begin();
                 execution_point_samples != thread_samples->value.per_execution_point_samples.end();
                 ++execution_point_samples) {
                // Collect samples
                if (!execution_point_samples->second.samples.empty()) {
                    std::vector<coro_sample_t> &sample_collection =
                        execution_point_reports[execution_point_samples->first].collected_samples;
                    sample_collection.insert(sample_collection.end(),
                                             execution_point_samples->second.samples.begin(),
                                             execution_point_samples->second.samples.end());

                    // Reset samples
                    execution_point_samples->second.samples.clear();
                }
            }
        }

        // Release per-thread locks
    }

    // Compute statistics
    for (auto report = execution_point_reports.begin();
         report != execution_point_reports.end();
         ++report) {
        report->second.compute_stats();
    }

    if (reql_output_file != nullptr) {
        print_to_reql(execution_point_reports);
    }
}

void coro_profiler_t::per_execution_point_collected_report_t::update_min_max(
    const double new_sample,
    data_distribution_t *current_out) const {

    if (num_samples == 0) {
        current_out->min = current_out->max = new_sample;
    } else {
        current_out->min = std::min(current_out->min, new_sample);
        current_out->max = std::max(current_out->max, new_sample);
    }
}

void coro_profiler_t::per_execution_point_collected_report_t::accumulate_sample_pass1(
    const double new_sample,
    data_distribution_t *current_out) const {

    update_min_max(new_sample, current_out);
    current_out->mean += new_sample;
}

void coro_profiler_t::per_execution_point_collected_report_t::divide_mean(
    data_distribution_t *current_out) const {

    current_out->mean /= (num_samples > 0) ? static_cast<double>(num_samples) : 1.0;
}

void coro_profiler_t::per_execution_point_collected_report_t::accumulate_sample_pass2(
    const double new_sample,
    data_distribution_t *current_out) const {

    current_out->stddev += std::pow(new_sample - current_out->mean, 2);
}

void coro_profiler_t::per_execution_point_collected_report_t::divide_stddev(
    data_distribution_t *current_out) const {

    // Use the unbiased estimate of the standard deviation, hence division by num_samples-1
    current_out->stddev /= (num_samples > 1) ? static_cast<double>(num_samples-1) : 1.0;
    current_out->stddev = std::sqrt(current_out->stddev);
}

void coro_profiler_t::per_execution_point_collected_report_t::compute_stats() {
    rassert(num_samples == 0); // `per_execution_point_collected_report_t` is not re-usable.

    // Pass 1: Compute min, max, mean
    for (auto sample = collected_samples.begin(); sample != collected_samples.end(); ++sample) {
        accumulate_sample_pass1(ticks_to_secs(sample->ticks_since_previous),
                                &time_since_previous);
        accumulate_sample_pass1(ticks_to_secs(sample->ticks_since_resume),
                                &time_since_resume);
        accumulate_sample_pass1(static_cast<double>(sample->priority),
                                &priority);

        ++num_samples;
    }
    divide_mean(&time_since_previous);
    divide_mean(&time_since_resume);
    divide_mean(&priority);

    // Pass 2: Compute standard deviation
    for (auto sample = collected_samples.begin(); sample != collected_samples.end(); ++sample) {
        accumulate_sample_pass2(ticks_to_secs(sample->ticks_since_previous),
                                &time_since_previous);
        accumulate_sample_pass2(ticks_to_secs(sample->ticks_since_resume),
                                &time_since_resume);
        accumulate_sample_pass2(static_cast<double>(sample->priority),
                                &priority);
    }
    divide_stddev(&time_since_previous);
    divide_stddev(&time_since_resume);
    divide_stddev(&priority);
}

void coro_profiler_t::print_to_reql(
    const std::map<coro_execution_point_key_t,
    per_execution_point_collected_report_t> &execution_point_reports) {
    guarantee(reql_output_file != nullptr);

    const double time = ticks_to_secs(get_ticks());

    for (auto report = execution_point_reports.begin(); report != execution_point_reports.end(); ++report) {
        fprintf(reql_output_file,
                "print t.insert({\n"
                "\t\t'time': %.10f,\n", time);
        fprintf(reql_output_file,
                "\t\t'coro_type': '%s',\n",
                report->first.first.c_str());
        fprintf(reql_output_file,
                "\t\t'trace': %s,\n",
                trace_to_array_str(report->first.second).c_str());
        fprintf(reql_output_file,
                "\t\t'num_samples': %zu,\n",
                report->second.num_samples);
        fprintf(reql_output_file,
                "\t\t'since_previous': %s,\n",
                distribution_to_object_str(report->second.time_since_previous).c_str());
        fprintf(reql_output_file,
                "\t\t'since_resume': %s,\n",
                distribution_to_object_str(report->second.time_since_resume).c_str());
        fprintf(reql_output_file,
                "\t\t'priority': %s\n",
                distribution_to_object_str(report->second.priority).c_str());
        fprintf(reql_output_file,
                "\t}).run(conn, durability='soft')\n");
    }
}

std::string coro_profiler_t::trace_to_array_str(const small_trace_t &trace) {
    std::string trace_array_str = "[";
    for (size_t i = 0; i < CORO_PROFILER_BACKTRACE_DEPTH; ++i) {
        if (trace[i] == NULL) {
            break;
        }
        if (i > 0) {
            trace_array_str += ", ";
        }
        trace_array_str += "'" + get_frame_description(trace[i]) + "'";
    }
    trace_array_str += "]";

    return trace_array_str;
}

std::string coro_profiler_t::distribution_to_object_str(const data_distribution_t &distribution) {
    return strprintf("{'min': %.10f, 'max': %.10f, 'mean': %.10f, 'stddev': %.10f}",
                     distribution.min, distribution.max,
                     distribution.mean, distribution.stddev);
}

void coro_profiler_t::write_reql_header() {
    guarantee(reql_output_file != nullptr);
    fprintf(reql_output_file,
            "#!/usr/bin/env python\n"
            "\n"
            "import rethinkdb as r\n"
            "conn = r.connect() # Modify this as needed\n"
            "t = r.table(\"coro_prof\") # Modify this as needed\n"
            "\n");
}

const std::string &coro_profiler_t::get_frame_description(void *addr) {
    auto cache_it = frame_description_cache.find(addr);
    if (cache_it != frame_description_cache.end()) {
        return cache_it->second;
    }

    backtrace_frame_t frame(addr);
    frame.initialize_symbols();


#if CORO_PROFILER_ADDRESS_TO_LINE
    std::string line = address_to_line.address_to_line(frame.get_filename(), frame.get_addr()) + "  |  ";
#else
    std::string line = "";
#endif
    std::string demangled_name;
    try {
        demangled_name = frame.get_demangled_name();
    } catch (const demangle_failed_exc_t &e) {
        demangled_name = "?";
    }
    std::string description = strprintf("%p\t%s%s",
                                        frame.get_addr(),
                                        line.c_str(),
                                        demangled_name.c_str());

    return frame_description_cache.insert(
        std::pair<void *, std::string>(addr, description)).first->second;
}

#endif /* ENABLE_CORO_PROFILER */
