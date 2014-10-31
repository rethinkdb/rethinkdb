// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/job_manager.hpp"

#include <functional>

#include "concurrency/watchable.hpp"
#include "stl_utils.hpp"

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(job_manager_business_card_t,
                                    get_job_wire_entries_mailbox_address);

job_manager_t::job_manager_t(mailbox_manager_t* mm) :
    mailbox_manager(mm),
    get_job_wire_entries_mailbox(mailbox_manager,
                          std::bind(&job_manager_t::on_get_job_wire_entries,
                                    this, ph::_1, ph::_2))
    { }

job_manager_business_card_t job_manager_t::get_business_card() {
    business_card_t business_card;
    business_card.get_job_wire_entries_mailbox_address =
        get_job_wire_entries_mailbox.get_address();
    return business_card;
}

void job_manager_t::on_get_job_wire_entries(
        UNUSED signal_t *interruptor,
        const business_card_t::return_mailbox_t::address_t& reply_address) {
    std::vector<job_wire_entry_t> job_wire_entries = get_job_wire_entries();
    send(mailbox_manager, reply_address, job_wire_entries);
}
