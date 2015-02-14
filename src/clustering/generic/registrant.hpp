// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_GENERIC_REGISTRANT_HPP_
#define CLUSTERING_GENERIC_REGISTRANT_HPP_

#include <algorithm>
#include <map>
#include <string>
#include <utility>

#include "clustering/generic/registration_metadata.hpp"
#include "clustering/generic/resource.hpp"
#include "containers/clone_ptr.hpp"
#include "containers/death_runner.hpp"
#include "containers/uuid.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/view.hpp"

template<class business_card_t>
class registrant_t {

public:
    /* This constructor registers with the given registrant. It may block for a short
    time. */
    registrant_t(
            mailbox_manager_t *mm,
            const registrar_business_card_t<business_card_t> &rmd,
            business_card_t initial_value) :
        mailbox_manager(mm),
        registrar_md(rmd),
        registration_id(generate_uuid())
    {
        /* This will make it so that we get deregistered in our destructor. */
        deregisterer.fun = std::bind(&registrant_t::send_deregister_message,
            mailbox_manager,
            registrar_md.delete_mailbox,
            registration_id);

        /* Send a message to register us */
        send(mailbox_manager, registrar_md.create_mailbox,
            registration_id,
            mailbox_manager->get_connectivity_cluster()->get_me(),
            initial_value);
    }

    /* The destructor deregisters us and returns immediately. It never throws
    any exceptions. */
    ~registrant_t() THROWS_NOTHING {

        /* Most of the work is done by the destructor for `deregisterer` */
    }

private:
    typedef typename registrar_business_card_t<business_card_t>::registration_id_t registration_id_t;

    /* We can't deregister in our destructor because then we wouldn't get
    deregistered if we died mid-constructor. Instead, the deregistration must be
    done by a separate subcomponent. `deregisterer` is that subcomponent. The
    constructor sets `deregisterer.fun` to a `std::bind()` of
    `send_deregister_message()`, and that deregisters things as necessary. */
    static void send_deregister_message(
            mailbox_manager_t *mailbox_manager,
            typename registrar_business_card_t<business_card_t>::delete_mailbox_t::address_t addr,
            registration_id_t rid) THROWS_NOTHING {
        send(mailbox_manager, addr, rid);
    }
    death_runner_t deregisterer;

    mailbox_manager_t *mailbox_manager;
    registrar_business_card_t<business_card_t> registrar_md;

    registration_id_t registration_id;

    DISABLE_COPYING(registrant_t);
};

#endif /* CLUSTERING_GENERIC_REGISTRANT_HPP_ */
