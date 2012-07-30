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
    /* This constructor registers with the given registrant. If the registrant
    is already dead, it throws an exception. Otherwise, it returns immediately.
    */
    registrant_t(
            mailbox_manager_t *mm,
            clone_ptr_t<watchable_t<boost::optional<boost::optional<registrar_business_card_t<business_card_t> > > > > registrar_md,
            business_card_t initial_value)
            THROWS_ONLY(resource_lost_exc_t) :
        mailbox_manager(mm),
        registrar(registrar_md),
        registration_id(generate_uuid())
    {
        /* This will make it so that we get deregistered in our destructor. */
        deregisterer.fun = boost::bind(&registrant_t::send_deregister_message,
            mailbox_manager,
            registrar.access().delete_mailbox,
            registration_id);

        /* Send a message to register us */
        send(mailbox_manager, registrar.access().create_mailbox,
            registration_id,
            mailbox_manager->get_connectivity_service()->get_me(),
            initial_value);
    }

    /* The destructor deregisters us and returns immediately. It never throws
    any exceptions. */
    ~registrant_t() THROWS_NOTHING {

        /* Most of the work is done by the destructor for `deregisterer` */
    }

    signal_t *get_failed_signal() THROWS_NOTHING {
        return registrar.get_failed_signal();
    }

    std::string get_failed_reason() THROWS_NOTHING {
        rassert(get_failed_signal()->is_pulsed());
        return registrar.get_failed_reason();
    }

private:
    typedef typename registrar_business_card_t<business_card_t>::registration_id_t registration_id_t;

    /* We can't deregister in our destructor because then we wouldn't get
    deregistered if we died mid-constructor. Instead, the deregistration must be
    done by a separate subcomponent. `deregisterer` is that subcomponent. The
    constructor sets `deregisterer.fun` to a `boost::bind()` of
    `send_deregister_message()`, and that deregisters things as necessary. */
    static void send_deregister_message(
            mailbox_manager_t *mailbox_manager,
            typename registrar_business_card_t<business_card_t>::delete_mailbox_t::address_t addr,
            registration_id_t rid) THROWS_NOTHING {
        send(mailbox_manager, addr, rid);
    }
    death_runner_t deregisterer;

    mailbox_manager_t *mailbox_manager;
    resource_access_t<registrar_business_card_t<business_card_t> > registrar;

    registration_id_t registration_id;

    DISABLE_COPYING(registrant_t);
};

#endif /* CLUSTERING_GENERIC_REGISTRANT_HPP_ */
