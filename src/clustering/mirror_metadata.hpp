#ifndef __CLUSTERING_MIRROR_METADATA_HPP__
#define __CLUSTERING_MIRROR_METADATA_HPP__

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/serialization/map.hpp>

#include "clustering/backfill_metadata.hpp"
#include "clustering/registration_metadata.hpp"
#include "clustering/resource.hpp"
#include "concurrency/fifo_checker.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/metadata/semilattice/map.hpp"
#include "timestamps.hpp"

/* `mirror_dispatcher_metadata_t` is the metadata that the master exposes to the
mirrors. */

template<class protocol_t>
class mirror_dispatcher_metadata_t {

public:
    /* The mirrors that the branch has are contained in `mirrors`. */

    typedef boost::uuids::uuid mirror_id_t;

    std::map<mirror_id_t, resource_metadata_t<backfiller_metadata_t<protocol_t> > > mirrors;

    /* When mirrors start up, they construct a `mirror_registration_t` and send
    it to the master via `registrar`. */

    class mirror_data_t {

    public:
        /* These are the types of mailboxes that the master uses to communicate with
        the mirrors. */

        typedef async_mailbox_t<void(
            typename protocol_t::write_t, transition_timestamp_t, order_token_t,
            async_mailbox_t<void()>::address_t
            )> write_mailbox_t;

        typedef async_mailbox_t<void(
            typename protocol_t::write_t, transition_timestamp_t, order_token_t,
            typename async_mailbox_t<void(typename protocol_t::write_response_t)>::address_t
            )> writeread_mailbox_t;

        typedef async_mailbox_t<void(
            typename protocol_t::read_t, state_timestamp_t, order_token_t,
            typename async_mailbox_t<void(typename protocol_t::read_response_t)>::address_t
            )> read_mailbox_t;

        /* The master sends a single message to `intro_mailbox` at the very
        beginning. This tells the mirror what timestamp it's at, and also tells
        it where to send an upgrade message. */

        typedef async_mailbox_t<void(
            typename writeread_mailbox_t::address_t,
            typename read_mailbox_t::address_t
            )> upgrade_mailbox_t;

        typedef async_mailbox_t<void(
            state_timestamp_t,
            typename upgrade_mailbox_t::address_t
            )> intro_mailbox_t;
        typename intro_mailbox_t::address_t intro_mailbox;

        typename write_mailbox_t::address_t write_mailbox;

        mirror_data_t() { }
        mirror_data_t(
                const typename intro_mailbox_t::address_t &im,
                const typename write_mailbox_t::address_t &wm) :
            intro_mailbox(im), write_mailbox(wm) { }

        RDB_MAKE_ME_SERIALIZABLE_2(intro_mailbox, write_mailbox);
    };

    resource_metadata_t<registrar_metadata_t<mirror_data_t> > registrar;

    RDB_MAKE_ME_SERIALIZABLE_2(mirrors, registrar);
};

template<class protocol_t>
void semilattice_join(mirror_dispatcher_metadata_t<protocol_t> *a, const mirror_dispatcher_metadata_t<protocol_t> &b) {
    semilattice_join(&a->mirrors, b.mirrors);
    semilattice_join(&a->registrar, b.registrar);
}

#endif /* __CLUSTERING_MIRROR_METADATA_HPP__ */
