#define __STDC_FORMAT_MACROS

#include "clustering/administration/proc_stats.hpp"

#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>

#include "arch/io/io_utils.hpp"   /* for `scoped_fd_t` and `_gettid()` */
#include "arch/timing.hpp"
#include "logger.hpp"
#include "containers/printf_buffer.hpp"

/* Class to represent and parse the contents of /proc/[pid]/stat */

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
            throw std::runtime_error(strprintf("Could not open '%s': %s (errno "
                "= %d)", path, strerror(errno), errno));
        }

        char buffer[1000];
        int res = ::read(stat_file.get(), buffer, sizeof(buffer));
        if (res <= 0) {
            throw std::runtime_error(strprintf("Could not read '%s': %s (errno "
                "= %d)", path, strerror(errno), errno));
        }

        buffer[res] = '\0';

#ifndef LEGACY_PROC_STAT
        const int items_to_parse = 44;
#else
        const int items_to_parse = 42;
#endif

        int res2 = sscanf(buffer, "%d %s %c %d %d %d %d %d %u"
                          " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                          " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64
                          " %" SCNu64 " %" SCNu64 " %" SCNd64
                          " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                          " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                          " %d %d"
                          " %u %u"
#ifndef LEGACY_PROC_STAT
                          " %" SCNu64
                          " %" SCNu64
                          " %" SCNd64,
#else
                          " %" SCNu64,
#endif
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
#ifndef LEGACY_PROC_STAT
                          &delayacct_blkio_ticks,
                          &guest_time,
                          &cguest_time
#else
                          &delayacct_blkio_ticks
#endif
                          );
        if (res2 != items_to_parse) {
            throw std::runtime_error(strprintf("Could not parse '%s': expected "
                "to parse %d items, parsed %d. Buffer contents: %s", path,
                items_to_parse, res2, buffer));
        }
    }
};

// This structure reads various global system stats such as total
// memory consumption, network stats, etc.
struct global_sys_stat_t {
public:
    int64_t mem_total, mem_free;
    int64_t utime, ntime, stime, itime, wtime;
    int ncpus;
    int64_t bytes_received, bytes_sent;

public:
    static global_sys_stat_t read_global_stats() {
        global_sys_stat_t stat;
        stat.mem_total = stat.mem_free = stat.utime = stat.ntime = stat.stime = stat.itime = stat.wtime = 0;
        stat.ncpus = 0;
        stat.bytes_received = stat.bytes_sent = 0;

        // Grab memory info
        {
            const char *path = "/proc/meminfo";
            scoped_fd_t stat_file(open(path, O_RDONLY));
            if (stat_file.get() == INVALID_FD) {
                throw std::runtime_error(strprintf("Could not open '%s': %s "
                    "(errno = %d)", path, strerror(errno), errno));
            }

            char buffer[1000];
            int res = ::read(stat_file.get(), buffer, sizeof(buffer));
            if (res <= 0) {
                throw std::runtime_error(strprintf("Could not read '%s': %s "
                    "(errno = %d)", path, strerror(errno), errno));
            }
            buffer[res] = '\0';

            char *_memtotal = strcasestr(buffer, "MemTotal");
            if (_memtotal) {
                res = sscanf(_memtotal, "MemTotal:%*[ ]%ld kB", &stat.mem_total);
            }
            char *_memfree = strcasestr(buffer, "MemFree");
            if (_memfree) {
                res = sscanf(_memfree, "MemFree:%*[ ]%ld kB", &stat.mem_free);
            }
        }

        // Grab CPU info
        {
            const char *path = "/proc/stat";
            scoped_fd_t stat_file(open(path, O_RDONLY));
            if (stat_file.get() == INVALID_FD) {
                throw std::runtime_error(strprintf("Could not open '%s': %s "
                    "(errno = %d)", path, strerror(errno), errno));
            }

            char buffer[1024 * 10];
            int res = ::read(stat_file.get(), buffer, sizeof(buffer));
            if (res <= 0) {
                throw std::runtime_error(strprintf("Could not read '%s': %s "
                    "(errno = %d)", path, strerror(errno), errno));
            }
            buffer[res] = '\0';

            res = sscanf(buffer, "cpu%*[ ]%ld %ld %ld %ld %ld",
                         &stat.utime, &stat.ntime, &stat.stime, &stat.itime, &stat.wtime);

            // Compute the number of cores
            char *core = buffer;
            do {
                core = strcasestr(core, "cpu");
                if (core) {
                    stat.ncpus++;
                    core += 3;
                }
            } while (core);
            stat.ncpus--; // the first line is an aggeragate
        }

        // Grab network info
        {
            const char *path = "/proc/net/dev";
            scoped_fd_t stat_file(open(path, O_RDONLY));
            if (stat_file.get() == INVALID_FD) {
                throw std::runtime_error(strprintf("Could not open '%s': %s "
                    "(errno = %d)", path, strerror(errno), errno));
            }

            char buffer[1024 * 10];
            int res = ::read(stat_file.get(), buffer, sizeof(buffer));
            if (res <= 0) {
                throw std::runtime_error(strprintf("Could not read '%s': %s "
                    "(errno = %d)", path, strerror(errno), errno));
            }
            buffer[res] = '\0';

            // Scan for bytes received and sent on each interface
            char *netinfo = buffer;
            do {
                netinfo = strstr(netinfo, ": ");
                if (netinfo) {
                    netinfo += 2;
                    int64_t recv, sent;
                    res = sscanf(netinfo, "%ld%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%*d%*[ ]%ld%*[ ]", &recv, &sent);
                    if (res == 2) {
                        stat.bytes_received += recv;
                        stat.bytes_sent += sent;
                    }
                }
            } while (netinfo);
            res = sscanf(buffer, "cpu%*[ ]%ld %ld %ld %ld %ld",
                         &stat.utime, &stat.ntime, &stat.stime, &stat.itime, &stat.wtime);
        }

        // Whoo, we're done.
        return stat;
    }
};

proc_stats_collector_t::proc_stats_collector_t(perfmon_collection_t *stats) :
    instantaneous_stats_collector(),
    cpu_thread_user(secs_to_ticks(5), false),
    cpu_thread_system(secs_to_ticks(5), false),
    cpu_thread_combined(secs_to_ticks(5), false),
    cpu_global_combined(secs_to_ticks(5), false),
    net_global_received(secs_to_ticks(5), false),
    net_global_sent(secs_to_ticks(5), false),
    memory_faults(secs_to_ticks(5), false),
    stats_membership(stats,
        &instantaneous_stats_collector, NULL,
        &cpu_thread_user, "cpu_user",
        &cpu_thread_system, "cpu_system",
        &cpu_thread_combined, "cpu_combined",
        &cpu_global_combined, "global_cpu_util",
        &net_global_received, "global_net_recv_persec",
        &net_global_sent, "global_net_sent_persec",
        &memory_faults, "memory_faults_persec",
        NULL)
{
    coro_t::spawn_sometime(boost::bind(&proc_stats_collector_t::collect_periodically, this, auto_drainer_t::lock_t(&drainer)));
}

proc_stats_collector_t::instantaneous_stats_collector_t::instantaneous_stats_collector_t() {
    // Whoever you are, please don't change this to start_time =
    // time(NULL) it results in a negative uptime stat.
    struct timespec now;
    int res = clock_gettime(CLOCK_MONOTONIC, &now);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTONIC) failed");
    start_time = now.tv_sec;
}

void *proc_stats_collector_t::instantaneous_stats_collector_t::begin_stats() {
    return NULL;
}

void proc_stats_collector_t::instantaneous_stats_collector_t::visit_stats(void *) {
    /* Do nothing; the things we need to get can be gotten on any thread */
}

perfmon_result_t *proc_stats_collector_t::instantaneous_stats_collector_t::end_stats(void *) {
    perfmon_result_t *result;
    perfmon_result_t::alloc_map_result(&result);

    // Basic process stats (version, pid, uptime)
    struct timespec now;
    int res = clock_gettime(CLOCK_MONOTONIC, &now);
    guarantee_err(res == 0, "clock_gettime(CLOCK_MONOTONIC) failed");

    result->insert("uptime", new perfmon_result_t(strprintf("%ld", now.tv_sec - start_time)));
    result->insert("timestamp", new perfmon_result_t(format_time(now)));

    result->insert("version", new perfmon_result_t(std::string(RETHINKDB_VERSION)));
    result->insert("pid", new perfmon_result_t(strprintf("%d", getpid())));

    try {
        proc_pid_stat_t pid_stat = proc_pid_stat_t::for_pid(getpid());
        result->insert("memory_virtual", new perfmon_result_t(strprintf("%lu", pid_stat.vsize)));
        result->insert("memory_real", new perfmon_result_t(strprintf("%ld", pid_stat.rss * sysconf(_SC_PAGESIZE))));

        global_sys_stat_t global_stat = global_sys_stat_t::read_global_stats();
        result->insert("global_mem_total", new perfmon_result_t(strprintf("%ld", global_stat.mem_total)));
        result->insert("global_mem_used", new perfmon_result_t(strprintf("%ld", global_stat.mem_total - global_stat.mem_free)));
    } catch (const std::runtime_error &e) {
        logWRN("Error in collecting system stats (on demand): %s", e.what());
    }
    return result;
}

static void fetch_tid(int thread, pid_t *out) {
    on_thread_t thread_switcher(thread);
    out[thread] = _gettid();
}

void proc_stats_collector_t::collect_periodically(auto_drainer_t::lock_t keepalive) {
    try {
        boost::scoped_array<pid_t> tids(new pid_t[get_num_threads()]);
        pmap(get_num_threads(), boost::bind(&fetch_tid, _1, tids.get()));

        ticks_t last_ticks = get_ticks();
        boost::scoped_array<proc_pid_stat_t> last_per_thread(new proc_pid_stat_t[get_num_threads()]);
        for (int i = 0; i < get_num_threads(); i++) {
            last_per_thread[i] = proc_pid_stat_t::for_pid_and_tid(getpid(), tids[i]);
        }
        global_sys_stat_t last_global = global_sys_stat_t::read_global_stats();

        try {
            for (;;) {
                signal_timer_t timer(1000);
                wait_interruptible(&timer, keepalive.get_drain_signal());

                ticks_t current_ticks = get_ticks();
                boost::scoped_array<proc_pid_stat_t> current_per_thread(new proc_pid_stat_t[get_num_threads()]);
                for (int i = 0; i < get_num_threads(); i++) {
                    current_per_thread[i] = proc_pid_stat_t::for_pid_and_tid(getpid(), tids[i]);
                }
                global_sys_stat_t current_global = global_sys_stat_t::read_global_stats();

                double elapsed_secs = ticks_to_secs(current_ticks - last_ticks);
                double elapsed_sys_ticks = elapsed_secs * sysconf(_SC_CLK_TCK);
                for (int i = 0; i < get_num_threads(); i++) {
                    cpu_thread_user.record(
                        (current_per_thread[i].utime - last_per_thread[i].utime) / elapsed_sys_ticks);
                    cpu_thread_system.record(
                        (current_per_thread[i].stime - last_per_thread[i].stime) / elapsed_sys_ticks);
                    cpu_thread_combined.record(
                        (current_per_thread[i].utime - last_per_thread[i].utime +
                         current_per_thread[i].stime - last_per_thread[i].stime) /
                         elapsed_sys_ticks);
                    memory_faults.record(
                        (current_per_thread[i].majflt - last_per_thread[i].majflt) / elapsed_secs);
                }
                cpu_global_combined.record(
                    (current_global.utime - last_global.utime +
                     current_global.stime - last_global.stime) /
                     elapsed_sys_ticks / current_global.ncpus);
                net_global_received.record(
                    (current_global.bytes_received - last_global.bytes_received) / elapsed_secs);
                net_global_sent.record(
                    (current_global.bytes_sent - last_global.bytes_sent) / elapsed_secs);

                last_ticks = current_ticks;
                swap(last_per_thread, current_per_thread);
                last_global = current_global;
            }
        } catch (interrupted_exc_t) {
            /* we're shutting down */
        }
    } catch (std::runtime_error e) {
        logWRN("Error in collecting system stats (on timer): %s", e.what());
    }
}
