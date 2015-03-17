// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLER_HPP_

#include <map>
#include <utility>

#include "clustering/immediate_consistency/history.hpp"
#include "clustering/immediate_consistency/backfill_metadata.hpp"

class backfiller_t : public home_thread_mixin_debug_only_t {
public:
    backfiller_t(mailbox_manager_t *mm,
                 branch_history_manager_t *bhm,
                 store_view_t *svs);

    backfiller_business_card_t get_business_card();

private:
    

    DISABLE_COPYING(backfiller_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BACKFILLER_HPP_ */
