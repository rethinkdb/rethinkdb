// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/sys_stats.hpp"

#include <inttypes.h>
#include <sys/statvfs.h>

#include "utils.hpp"

struct disk_stat_t {
    int64_t disk_space_free;
    int64_t disk_space_used;
    int64_t disk_space_total;

    explicit disk_stat_t(const std::string &filepath) {
        int res;
        // get disk space data using statvfs
        struct statvfs fsdata;

        res = statvfs(filepath.c_str(), &fsdata);

        if (res < 0) {
            disk_space_total = -1;
            disk_space_free = -1;
            disk_space_used = -1;
        }

        disk_space_total = fsdata.f_bsize * fsdata.f_blocks;
        disk_space_free = fsdata.f_bsize * fsdata.f_bfree;
        disk_space_used = disk_space_total - disk_space_free;
    }
};

sys_stats_collector_t::sys_stats_collector_t(const base_path_t &path, perfmon_collection_t *stats) :
    instantaneous_stats_collector(path),
    stats_membership(stats, &instantaneous_stats_collector, NULL) {
}

sys_stats_collector_t::instantaneous_stats_collector_t::instantaneous_stats_collector_t(const base_path_t &path) :
    base_path(path) {
}

void *sys_stats_collector_t::instantaneous_stats_collector_t::begin_stats() {
    return NULL;
}

void sys_stats_collector_t::instantaneous_stats_collector_t::visit_stats(void *) { }

ql::datum_t sys_stats_collector_t::instantaneous_stats_collector_t::end_stats(void *) {
    ql::datum_object_builder_t builder;

    disk_stat_t disk_stat(base_path.path());
    builder.overwrite("global_disk_space_free",
                      ql::datum_t(static_cast<double>(disk_stat.disk_space_free)));
    builder.overwrite("global_disk_space_used",
                      ql::datum_t(static_cast<double>(disk_stat.disk_space_used)));
    builder.overwrite("global_disk_space_total",
                      ql::datum_t(static_cast<double>(disk_stat.disk_space_total)));

    return std::move(builder).to_datum();
}
