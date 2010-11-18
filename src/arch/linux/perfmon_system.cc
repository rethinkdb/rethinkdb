/* This file exists to provide some stat monitors for process statistics and the like. */

#include "perfmon.hpp"
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
/* Class to represent and parse the contents of /proc/[pid]/stat */

struct proc_pid_stat_t {

    int pid;
    char name[500];
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned int flags;
    unsigned long int minflt, cminflt, majflt, cmajflt, utime, stime;
    long int cutime, cstime, priority, nice, num_threads, itrealvalue;
    long long unsigned int starttime;
    long unsigned int vsize;
    long int rss;
    long unsigned int rsslim, startcode, endcode, startstack, kstkesp, kstkeip, signal, blocked,
        sigignore, sigcatch, wchan, nswap, cnswap;
    int exit_signal, processor;
    unsigned int rt_priority, policy;
    long long unsigned int delayacct_blkio_ticks;
    long unsigned int guest_time;
    long int cguest_time;
    
    void read(const char *path) {
        
        int stat_file = open(path, O_RDONLY);
        if (stat_file == -1) fail("Could not open '%s'", path);
        
        char buffer[1000];
        int res = ::read(stat_file, buffer, 1000);
        if (res <= 0) fail("Could not read '%s': %s", path, strerror(errno));
        buffer[res] = '\0';
        
        close(stat_file);
        
        int res2 = sscanf(buffer, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld "
            "%ld %ld %ld %ld %llu %lu %ld %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %d "
            "%d %u %u %u %u %llu %lu %ld",
            &pid, name, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
            &flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime,
            &cutime, &cstime, &priority, &nice, &num_threads, &itrealvalue,
            &starttime, &vsize, &rss, &rsslim, &startcode, &endcode, &startstack,
            &kstkesp, &kstkeip, &signal, &blocked, &sigignore, &sigcatch, &wchan,
            &nswap, &cnswap, &exit_signal, &processor, &rt_priority, &processor,
            &rt_priority, &policy, &delayacct_blkio_ticks, &guest_time, &cguest_time);
        if (res2 < 44) fail("Could not parse \"%s\"; res=%d", buffer, res2);
    }
};

/* perfmon_system_t is used to monitor system stats that do not need to be polled. */

class perfmon_system_t :
    public perfmon_t
{
    void *begin_stats() {
        return NULL;
    }
    void visit_stats(void *) {
    }
    void end_stats(void *, perfmon_stats_t *dest) {
        
        char proc_pid_stat_path[100];
        sprintf(proc_pid_stat_path, "/proc/%d/stat", getpid());
        proc_pid_stat_t pid_stat;
        pid_stat.read(proc_pid_stat_path);
        (*dest)["pid"] = format(pid_stat.pid);
        (*dest)["memory_virtual[bytes]"] = format(pid_stat.vsize);
        (*dest)["memory_real[bytes]"] = format(pid_stat.rss * sysconf(_SC_PAGESIZE));
    }
} pm_system;

/* Some of the stats need to be polled periodically. Call this function periodically on each
thread to ensure that stats are up to date. It takes a void* so that it can be called as a timer
callback. */

perfmon_sampler_t
    pm_cpu_user("cpu_user", secs_to_ticks(5)),
    pm_cpu_system("cpu_system", secs_to_ticks(5)),
    pm_memory_faults("memory_faults", secs_to_ticks(5));

static __thread proc_pid_stat_t last_stats;
static __thread ticks_t last_ticks = 0;

void poll_system_stats(void *) {
    
    char proc_pid_task_tid_stat_path[100];
    pid_t tid = syscall(SYS_gettid);
    sprintf(proc_pid_task_tid_stat_path, "/proc/%d/task/%d/stat", getpid(), tid);
    proc_pid_stat_t current_stats;
    current_stats.read(proc_pid_task_tid_stat_path);
    ticks_t current_ticks = get_ticks();
    
    if (last_ticks == 0) {
        last_stats = current_stats;
        last_ticks = current_ticks;
    
    } else if (current_ticks > last_ticks + secs_to_ticks(1)) {
        
        double realtime_elapsed = ticks_to_secs(current_ticks - last_ticks) * sysconf(_SC_CLK_TCK);
        pm_cpu_user.record((current_stats.utime - last_stats.utime) / realtime_elapsed);
        pm_cpu_system.record((current_stats.stime - last_stats.stime) / realtime_elapsed);
        pm_memory_faults.record((current_stats.majflt - last_stats.majflt) / realtime_elapsed);
        
        last_stats = current_stats;
        last_ticks = current_ticks;
    }
}

