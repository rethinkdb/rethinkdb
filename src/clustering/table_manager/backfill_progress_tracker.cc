// Copyright 2010-2015 RethinkDB, all rights reserved.

#include "clustering/table_manager/backfill_progress_tracker.hpp"

backfill_progress_tracker_t::progress_tracker_t *
backfill_progress_tracker_t::insert_progress_tracker(const region_t &region) {
    return &progress_trackers.get()->insert(
            std::make_pair(region, backfill_progress_tracker_t::progress_tracker_t{})
        ).first->second;
}

std::map<region_t, backfill_progress_tracker_t::progress_tracker_t>
backfill_progress_tracker_t::get_progress_trackers() {
    // TODO

    std::map<region_t, progress_tracker_t> output;
    pmap(get_num_threads(), [&](int i) {
        std::map<region_t, progress_tracker_t> temp;
        {
            on_thread_t th((threadnum_t(i)));
            temp = *progress_trackers.get();
        }
        for (const auto &pair : temp) {
            auto res = output.insert(pair);
            guarantee(res.second);
        }
    });
    return output;
}
