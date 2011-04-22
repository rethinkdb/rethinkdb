#include "arch/linux/disk/stats.hpp"

perfmon_duration_sampler_t
    pm_io_reads("io_disk_reads", secs_to_ticks(1)),
    pm_io_writes("io_disk_writes", secs_to_ticks(1));