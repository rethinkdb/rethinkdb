#include "clustering/immediate_consistency/branch/replier.hpp"

#include "clustering/immediate_consistency/branch/listener.hpp"
#include "rpc/semilattice/view.hpp"

template <class protocol_t>
replier_t<protocol_t>::replier_t(listener_t<protocol_t> *l) :
    listener(l),

    synchronize_mailbox(listener->mailbox_manager,
			boost::bind(&replier_t<protocol_t>::on_synchronize,
				    this,
				    _1,
				    _2,
				    auto_drainer_t::lock_t(&drainer))),

    /* Start serving backfills */
    backfiller(listener->mailbox_manager,
	       listener->branch_history,
	       listener->svs)
{
    rassert(listener->svs->get_multistore_joined_region() ==
            listener->branch_history->get().branches[listener->branch_id].region,
            "Even though you can have a listener that only watches some subset "
            "of a branch, you can't have a replier for some subset of a "
            "branch.");

    /* Notify the broadcaster that we can reply to queries */
    send(listener->mailbox_manager,
	 listener->registration_done_cond.get_value().upgrade_mailbox,
	 listener->writeread_mailbox.get_address(),
	 listener->read_mailbox.get_address()
	 );
}

template <class protocol_t>
replier_t<protocol_t>::~replier_t() {
    if (listener->get_broadcaster_lost_signal()->is_pulsed()) {
	send(listener->mailbox_manager,
	     listener->registration_done_cond.get_value().downgrade_mailbox,
	     /* We don't want a confirmation */
	     mailbox_addr_t<void()>()
	     );
    }
}

template <class protocol_t>
replier_business_card_t<protocol_t> replier_t<protocol_t>::get_business_card() {
    return replier_business_card_t<protocol_t>(synchronize_mailbox.get_address(), backfiller.get_business_card());
}

template <class protocol_t>
void replier_t<protocol_t>::on_synchronize(state_timestamp_t timestamp, mailbox_addr_t<void()> ack_mbox, auto_drainer_t::lock_t keepalive) {
    try {
	listener->wait_for_version(timestamp, keepalive.get_drain_signal());
	send(listener->mailbox_manager, ack_mbox);
    } catch (interrupted_exc_t) {
    }
}


#include "mock/dummy_protocol.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class replier_t<memcached_protocol_t>;
template class replier_t<mock::dummy_protocol_t>;
template class replier_t<rdb_protocol_t>;
