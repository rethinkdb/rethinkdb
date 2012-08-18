#include "clustering/administration/sys_stats.hpp"
#include "errors.hpp"

#include <sys/statvfs.h>

struct disk_stat_t {
    uint64_t disk_space_free;
    uint64_t disk_space_used;
    uint64_t disk_space_total;

    disk_stat_t(const std::string &filepath) {
        int res;
        // get disk space data using statvfs
        struct statvfs fsdata;

        if (filepath == "") {
            res = statvfs(".", &fsdata);
        } else {
            res = statvfs(filepath.c_str(), &fsdata);
        }
        if (res < 0) {
            throw std::runtime_error(strprintf("Failed to statvfs with filepath '%s': %s "
                "(errno = %d)", filepath.c_str(), strerror(errno), errno));
        }

        disk_space_total = fsdata.f_bsize * fsdata.f_blocks / KILOBYTE;
        disk_space_free = fsdata.f_bsize * fsdata.f_bfree / KILOBYTE;
        disk_space_used = disk_space_total - disk_space_free;
    }
};

sys_stats_collector_t::sys_stats_collector_t(const std::string &path, perfmon_collection_t *stats) :
    instantaneous_stats_collector(path),
    stats_membership(stats, &instantaneous_stats_collector, NULL)
{
}

sys_stats_collector_t::instantaneous_stats_collector_t::instantaneous_stats_collector_t(const std::string &path) :
    filepath(path)
{
}

void *sys_stats_collector_t::instantaneous_stats_collector_t::begin_stats() {
    return NULL;
}

void sys_stats_collector_t::instantaneous_stats_collector_t::visit_stats(void *) {
}

perfmon_result_t *sys_stats_collector_t::instantaneous_stats_collector_t::end_stats(void *) {
    perfmon_result_t *result;
    perfmon_result_t::alloc_map_result(&result);

    disk_stat_t disk_stat = disk_stat_t(filepath);
    result->insert("global_disk_space_free", new perfmon_result_t(strprintf("%lu", disk_stat.disk_space_free)));
    result->insert("global_disk_space_used", new perfmon_result_t(strprintf("%lu", disk_stat.disk_space_used)));
    result->insert("global_disk_space_total", new perfmon_result_t(strprintf("%lu", disk_stat.disk_space_total)));

    return result;
}
