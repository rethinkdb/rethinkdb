// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/replier.hpp"

#include "clustering/immediate_consistency/listener.hpp"
#include "rpc/semilattice/view.hpp"
#include "store_view.hpp"

replier_t::replier_t(listener_t *li,
                     mailbox_manager_t *mailbox_manager,
                     branch_history_manager_t *branch_history_manager) :
    mailbox_manager_(mailbox_manager),
    listener_(li),

    synchronize_mailbox_(mailbox_manager_,
                         std::bind(&replier_t::on_synchronize,
                                   this,
                                   ph::_1,
                                   ph::_2,
                                   ph::_3)),

    /* Start serving backfills */
    backfiller_(mailbox_manager_,
                branch_history_manager,
                listener_->svs()) {

#ifndef NDEBUG
    {
        // TODO: Lose the need to switch threads for this assertion.
        region_t svs_region = listener_->svs()->get_region();
        on_thread_t th(branch_history_manager->home_thread());
        guarantee(svs_region == branch_history_manager->get_branch(listener_->branch_id()).region,
                           "Even though you can have a listener that only watches some subset "
                           "of a branch, you can't have a replier for some subset of a "
                           "branch.");
    }
#endif  // NDEBUG

    /* Notify the broadcaster that we can reply to queries */
    send(mailbox_manager_,
         listener_->registration_done_cond_value().upgrade_mailbox,
         listener_->writeread_address(),
         listener_->read_address());
}

replier_t::~replier_t() {
    send(mailbox_manager_,
         listener_->registration_done_cond_value().downgrade_mailbox,
         /* We don't want a confirmation */
         mailbox_addr_t<void()>());
}

replier_business_card_t replier_t::get_business_card() {
    return replier_business_card_t(synchronize_mailbox_.get_address(), backfiller_.get_business_card());
}

void replier_t::on_synchronize(
        signal_t *interruptor,
        state_timestamp_t timestamp,
        mailbox_addr_t<void()> ack_mbox) {
    try {
        listener_->wait_for_version(timestamp, interruptor);
        send(mailbox_manager_, ack_mbox);
    } catch (const interrupted_exc_t &) {
    }
}

