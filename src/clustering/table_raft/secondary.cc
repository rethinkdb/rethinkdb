// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_raft/secondary.hpp"

void follower_t::execute_contract_secondary(
        const region_t &region,
        const contract_id_t &cid,
        const contract_t &c,
        store_t *store,
        ongoing_secondary_t *ongoing,
        signal_t *interruptor) {
    boost::optional<branch_id_t> branch =
        static_cast<bool>(c.primary) ? boost::make_optional(c.branch) : boost::none;
    if (ongoing->branch != branch) {
        ongoing->replier.reset();
        ongoing->listener.reset();
    }
    ongoing->branch = branch;
    if (static_cast<bool>(branch)) {
        if (!ongoing->listener.has()) {
            broadcaster_business_card_t bcard = find_broadcaster(c.branch, interruptor);
            ongoing->listener.init(new listener_t(
                ...,
                bcard,
                ...));
        }
        if (!ongoing->replier.has()) {
            ongoing->replier.init(new replier_t(
                ...));
        }
        wait_interruptible(
            ongoing->listener->get_broadcaster_lost_signal(),
            interruptor);
        ongoing->replier.reset();
        ongoing->listener.reset();
    }
}
