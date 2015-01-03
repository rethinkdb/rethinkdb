// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLER_HPP_

#include <map>
#include <utility>

#include "clustering/immediate_consistency/branch/history.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"

class backfiller_send_backfill_callback_t;
template <class> class semilattice_read_view_t;
class traversal_progress_combiner_t;

/* If you construct a `backfiller_t` for a given store, then it will advertise
its existence in the metadata and serve backfills over the network. Generally
`backfiller_t` is constructed as a member of `replier_t`. */

class backfiller_t : public home_thread_mixin_debug_only_t {
public:
    backfiller_t(mailbox_manager_t *mm,
                 branch_history_manager_t *bhm,
                 store_view_t *svs);

    backfiller_business_card_t get_business_card();

    /* TODO: Support warm shutdowns? */

private:
    friend class backfiller_send_backfill_callback_t;

    bool confirm_and_send_metainfo(region_map_t<binary_blob_t> metainfo,
                                   region_map_t<version_range_t> start_point,
                                   mailbox_addr_t<void(region_map_t<version_range_t>, branch_history_t)> end_point_cont);

    void on_backfill(
            signal_t *interruptor,
            backfill_session_id_t session_id,
            const region_map_t<version_range_t> &start_point,
            const branch_history_t &start_point_associated_branch_history,
            mailbox_addr_t<void(region_map_t<version_range_t>, branch_history_t)> end_point_cont,
            mailbox_addr_t<void(
                backfill_chunk_t,
                double,
                fifo_enforcer_write_token_t
                )> chunk_cont,
            mailbox_addr_t<void(fifo_enforcer_write_token_t)> done_cont,
            mailbox_addr_t<void(mailbox_addr_t<void(int)>)> allocation_registration_box);

    void on_cancel_backfill(signal_t *interruptor, backfill_session_id_t session_id);

    mailbox_manager_t *const mailbox_manager;
    branch_history_manager_t *const branch_history_manager;

    store_view_t *const svs;

    std::map<backfill_session_id_t, cond_t *> local_interruptors;

    backfiller_business_card_t::backfill_mailbox_t backfill_mailbox;
    backfiller_business_card_t::cancel_backfill_mailbox_t cancel_backfill_mailbox;

    DISABLE_COPYING(backfiller_t);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BACKFILLER_HPP_ */
