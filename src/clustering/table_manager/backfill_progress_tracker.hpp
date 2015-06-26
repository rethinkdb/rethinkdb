// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_TABLE_MANAGER_BACKFILL_PROGRESS_TRACKER_HPP_
#define CLUSTERING_TABLE_MANAGER_BACKFILL_PROGRESS_TRACKER_HPP_

#include <map>

#include "concurrency/one_per_thread.hpp"
#include "containers/uuid.hpp"
#include "region/region.hpp"
#include "time.hpp"

class backfill_progress_tracker_t {
public:
    class progress_tracker_t {
    public:
        bool is_ready;
        microtime_t start_time;
        server_id_t source_server_id;
        double progress;
    };

    progress_tracker_t * insert_progress_tracker(const region_t &region);

    std::map<region_t, backfill_progress_tracker_t::progress_tracker_t>
        get_progress_trackers();

private:
    one_per_thread_t<std::map<
        region_t, backfill_progress_tracker_t::progress_tracker_t> > progress_trackers;
};

#endif /* CLUSTERING_TABLE_MANAGER_BACKFILL_PROGRESS_TRACKER_HPP_ */

