/* This file exists to provide some stat monitors for process statistics and the like. */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#include <vector>

#include "arch/io/io_utils.hpp"
#include "containers/printf_buffer.hpp"
#include "perfmon.hpp"
#include "logger.hpp"
#include "utils.hpp"
#include "thread_local.hpp"

/* Class to represent and parse the contents of /proc/[pid]/stat */

struct proc_pid_stat_exc_t : public std::exception {
    explicit proc_pid_stat_exc_t(const char *format, ...) __attribute__ ((format (printf, 2, 3))) {
        va_list ap;
        va_start(ap, format);
        msg = vstrprintf(format, ap);
        va_end(ap);
    }
    std::string msg;
    const char *what() const throw () {
        return msg.c_str();
    }
    ~proc_pid_stat_exc_t() throw () { }
};

struct proc_pid_stat_t {
    int pid;
    char name[500];
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned int flags;
    uint64_t minflt, cminflt, majflt, cmajflt, utime, stime;
    int64_t cutime, cstime, priority, nice, num_threads, itrealvalue;
    uint64_t starttime;
    uint64_t vsize;
    int64_t rss;
    uint64_t rsslim, startcode, endcode, startstack, kstkesp, kstkeip, signal, blocked,
        sigignore, sigcatch, wchan, nswap, cnswap;
    int exit_signal, processor;
    unsigned int rt_priority, policy;
    uint64_t delayacct_blkio_ticks;
    uint64_t guest_time;
    int64_t cguest_time;

    static proc_pid_stat_t for_pid(pid_t pid) {
        printf_buffer_t<100> path("/proc/%d/stat", pid);
        proc_pid_stat_t stat;
        stat.read_from_file(path.c_str());
        return stat;
    }

    static proc_pid_stat_t for_pid_and_tid(pid_t pid, pid_t tid) {
        printf_buffer_t<100> path("/proc/%d/task/%d/stat", pid, tid);
        proc_pid_stat_t stat;
        stat.read_from_file(path.c_str());
        return stat;
    }

private:
    void read_from_file(const char * path) {
        scoped_fd_t stat_file(open(path, O_RDONLY));
        if (stat_file.get() == INVALID_FD) {
            throw proc_pid_stat_exc_t("Could not open '%s': %s (errno = %d)", path, strerror(errno), errno);
        }

        char buffer[1000];
        int res = ::read(stat_file.get(), buffer, sizeof(buffer));
        if (res <= 0) {
            throw proc_pid_stat_exc_t("Could not read '%s': %s (errno = %d)", path, strerror(errno), errno);
        }

        buffer[res] = '\0';

        const int items_to_parse = 44;
        int res2 = sscanf(buffer, "%d %s %c %d %d %d %d %d %u"
                          " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                          " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64
                          " %" SCNu64 " %" SCNu64 " %" SCNd64
                          " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                          " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                          " %d %d"
                          " %u %u"
                          " %" SCNu64
                          " %" SCNu64
                          " %" SCNd64,
                          &pid,
                          name,
                          &state,
                          &ppid, &pgrp, &session, &tty_nr, &tpgid,
                          &flags,
                          &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime,
                          &cutime, &cstime, &priority, &nice, &num_threads, &itrealvalue,
                          &starttime,
                          &vsize,
                          &rss,
                          &rsslim, &startcode, &endcode, &startstack, &kstkesp, &kstkeip, &signal, &blocked,
                          &sigignore, &sigcatch, &wchan, &nswap, &cnswap,
                          &exit_signal, &processor,
                          &rt_priority, &policy,
                          &delayacct_blkio_ticks,
                          &guest_time,
                          &cguest_time);
        if (res2 != items_to_parse) {
            throw proc_pid_stat_exc_t("Could not parse '%s': expected to parse %d items, parsed "
                "%d. Buffer contents: %s", path, items_to_parse, res2, buffer);
        }
    }
};

/* perfmon_system_t is used to monitor system stats that do not need to be polled. */

struct perfmon_system_t :
    public perfmon_t
{
    bool have_reported_error;
    perfmon_system_t() : perfmon_t(false), have_reported_error(false), start_time(time(NULL)) { }

    void *begin_stats() {
        return NULL;
    }
    void visit_stats(void *) {
    }
    void end_stats(void *, perfmon_stats_t *dest) {
        put_timestamp(dest);
        (*dest)["version"] = std::string(RETHINKDB_VERSION);
        (*dest)["pid"] = strprintf("%d", getpid());

        proc_pid_stat_t pid_stat;
        try {
            pid_stat = proc_pid_stat_t::for_pid(getpid());
        } catch (proc_pid_stat_exc_t e) {
            if (!have_reported_error) {
                logWRN("Error in reporting system stats: %s (Further errors like this will "
                    "be suppressed.)\n", e.what());
                have_reported_error = true;
            }
            return;
        }

        (*dest)["memory_virtual[bytes]"] = strprintf("%lu", pid_stat.vsize);
        (*dest)["memory_real[bytes]"] = strprintf("%ld", pid_stat.rss * sysconf(_SC_PAGESIZE));
    }
    void put_timestamp(perfmon_stats_t *dest) {
        time_t now = time(NULL);
        (*dest)["uptime"] = strprintf("%d", now - start_time);
        (*dest)["timestamp"] = format_time(now);
    }

    time_t start_time;
} pm_system;

/* Some of the stats need to be polled periodically. Call this function periodically on each
thread to ensure that stats are up to date. It takes a void* so that it can be called as a timer
callback. */

perfmon_sampler_t
    pm_cpu_user("cpu_user", secs_to_ticks(5)),
    pm_cpu_system("cpu_system", secs_to_ticks(5)),
    pm_cpu_combined("cpu_combined", secs_to_ticks(5)),
    pm_memory_faults("memory_faults", secs_to_ticks(5));


TLS(proc_pid_stat_t, last_stats);
TLS_with_init(ticks_t, last_ticks, 0);
TLS_with_init(bool, have_reported_stats_error, false);

void poll_system_stats(void *) {

    proc_pid_stat_t current_stats;
    try {
        current_stats = proc_pid_stat_t::for_pid_and_tid(getpid(), syscall(SYS_gettid));
    } catch (proc_pid_stat_exc_t e) {
        if (!TLS_get_have_reported_stats_error()) {
            logWRN("Error in reporting per-thread stats: %s (Further errors like this will "
                "be suppressed.)\n", e.what());
            TLS_set_have_reported_stats_error(true);
        }
    }
    ticks_t current_ticks = get_ticks();

    if (TLS_get_last_ticks() == 0) {
        TLS_set_last_stats(current_stats);
        TLS_set_last_ticks(current_ticks);
    } else if (current_ticks > TLS_get_last_ticks() + secs_to_ticks(1)) {
        double realtime_elapsed = ticks_to_secs(current_ticks - TLS_get_last_ticks()) * sysconf(_SC_CLK_TCK);
        pm_cpu_user.record((current_stats.utime - TLS_get_last_stats().utime) / realtime_elapsed);
        pm_cpu_system.record((current_stats.stime - TLS_get_last_stats().stime) / realtime_elapsed);
        pm_cpu_combined.record(
            (current_stats.utime - TLS_get_last_stats().utime +
             current_stats.stime - TLS_get_last_stats().stime) /
             realtime_elapsed);
        pm_memory_faults.record((current_stats.majflt - TLS_get_last_stats().majflt) / realtime_elapsed);
        
        TLS_set_last_stats(current_stats);
        TLS_set_last_ticks(current_ticks);
    }
}

