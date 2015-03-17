// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_

#include "clustering/immediate_consistency/history.hpp"
#include "clustering/immediate_consistency/backfill_metadata.hpp"
#include "rpc/connectivity/peer_id.hpp"

class backfillee_t : public home_thread_mixin_debug_only_t {
public:
    backfillee_t(
        mailbox_manager_t *mailbox_manager,
        const backfiller_bcard_t &backfiller,
        store_view_t *store,
        const region_map_t<version_t> &goal);
    void update_goal(
        const region_map_t<version_t> &new_goal);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLEE_HPP_ */
