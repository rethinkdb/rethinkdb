// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/immediate_consistency/branch/replier.hpp"

#include "clustering/immediate_consistency/branch/listener.hpp"
#include "rpc/semilattice/view.hpp"

template <class protocol_t>
replier_t<protocol_t>::replier_t(listener_t<protocol_t> *li,
                                 mailbox_manager_t *mailbox_manager,
                                 branch_history_manager_t<protocol_t> *branch_history_manager) :
    mailbox_manager_(mailbox_manager),
    listener_(li),

    synchronize_mailbox_(mailbox_manager_,
                         std::bind(&replier_t<protocol_t>::on_synchronize,
                                   this,
                                   ph::_1,
                                   ph::_2,
                                   auto_drainer_t::lock_t(&drainer_))),

    /* Start serving backfills */
    backfiller_(mailbox_manager_,
                branch_history_manager,
                listener_->svs()) {

#ifndef NDEBUG
    {
        // TODO: Lose the need to switch threads for this assertion.
        typename protocol_t::region_t svs_region = listener_->svs()->get_region();
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

template <class protocol_t>
replier_t<protocol_t>::~replier_t() {
    if (listener_->get_broadcaster_lost_signal()->is_pulsed()) {
        send(mailbox_manager_,
             listener_->registration_done_cond_value().downgrade_mailbox,
             /* We don't want a confirmation */
             mailbox_addr_t<void()>());
    }
}

template <class protocol_t>
replier_business_card_t<protocol_t> replier_t<protocol_t>::get_business_card() {
    return replier_business_card_t<protocol_t>(synchronize_mailbox_.get_address(), backfiller_.get_business_card());
}

template <class protocol_t>
void replier_t<protocol_t>::on_synchronize(state_timestamp_t timestamp, mailbox_addr_t<void()> ack_mbox, auto_drainer_t::lock_t keepalive) {
    try {
        listener_->wait_for_version(timestamp, keepalive.get_drain_signal());
        send(mailbox_manager_, ack_mbox);
    } catch (const interrupted_exc_t &) {
    }
}


#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class replier_t<memcached_protocol_t>;
template class replier_t<mock::dummy_protocol_t>;
template class replier_t<rdb_protocol_t>;
