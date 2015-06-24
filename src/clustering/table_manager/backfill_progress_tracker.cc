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
    std::map<region_t, progress_tracker_t> output;
    pmap(get_num_threads(), [&](int thread) {
        std::map<region_t, progress_tracker_t> inner;
        {
            on_thread_t on_thread((threadnum_t(thread)));
            inner = *progress_trackers.get();
        }
        for (const auto &pair : inner) {
            auto res = output.insert(std::move(pair));
            guarantee(res.second);
        }
    });
    return output;
}
