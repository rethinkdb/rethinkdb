#ifndef __CLUSTERING_MIRROR_METADATA_HPP__
#define __CLUSTERING_MIRROR_METADATA_HPP__

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/serialization/map.hpp>

#include "clustering/registration_metadata.hpp"
#include "clustering/resource.hpp"
#include "concurrency/fifo_checker.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/metadata/semilattice/map.hpp"
#include "timestamps.hpp"

/* `branch_metadata_t` is the metadata that the master exposes to the
mirrors. */

typedef boost::uuids::uuid mirror_id_t;

/* Every `listener_t` constructs a `listener_data_t` and sends it to the
`broadcaster_t`. */

template<class protocol_t>
class listener_data_t {

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
    it where to send upgrade/downgrade messages. */

    typedef async_mailbox_t<void(
        typename writeread_mailbox_t::address_t,
        typename read_mailbox_t::address_t
        )> upgrade_mailbox_t;

    typedef async_mailbox_t<void(
        async_mailbox_t<void()>::address_t
        )> downgrade_mailbox_t;

    typedef async_mailbox_t<void(
        state_timestamp_t,
        typename upgrade_mailbox_t::address_t,
        typename downgrade_mailbox_t::address_t
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

/* When the `listener_t` is upgraded to a `mirror_t`, it constructs a
`mirror_metadata_t` and puts it in the metadata. */

template<class protocol_t>
class mirror_metadata_t {

    typedef boost::uuids::uuid backfill_session_id_t;

    typedef async_mailbox_t<void(
        backfill_session_id_t,
        std::vector<std::pair<typename protocol_t::region_t, version_range_t> >,
        typename async_mailbox_t<void(version_map_t<protocol_t>)>::address_t,
        typename async_mailbox_t<void(typename protocol_t::store_t::backfill_chunk_t)>::address_t,
        typename async_mailbox_t<void(state_timestamp_t)>::address_t
        )> backfill_mailbox_t;
    typename backfill_mailbox_t::address_t backfill_mailbox;

    typedef async_mailbox_t<void(
        backfill_session_id_t
        )> cancel_backfill_mailbox_t;
    cancel_backfill_mailbox_t::address_t cancel_backfill_mailbox;
};

template<class protocol_t>
void semilattice_join(UNUSED mirror_metadata_t<protocol_t> *a, UNUSED const mirror_metadata_t<protocol_t> &b) {
    /* They should be equal, but we don't bother testing that */
}

/* There is one `branch_metadata_t` per branch. It's created by the broadcaster.
Listeners use it to find the broadcaster and a backfiller. */

template<class protocol_t>
class branch_metadata_t {

public:
    /* When listeners start up, they construct a `listener_data_t` and send it
    to the broadcaster via `broadcaster_registrar` */

    resource_metadata_t<registrar_metadata_t<listener_data_t<protocol_t> > > broadcaster_registrar;

    /* When listeners are upgraded to mirrors, they make an entry in `mirrors`.
    */
    std::map<mirror_id_t, resource_metadata_t<mirror_metadata_t<protocol_t> > > mirrors;

    RDB_MAKE_ME_SERIALIZABLE_2(broadcaster_registrar, mirrors);
};

template<class protocol_t>
void semilattice_join(branch_metadata_t<protocol_t> *a, const branch_metadata_t<protocol_t> &b) {
    semilattice_join(&a->broadcaster_registrar, b.broadcaster_registrar);
    semilattice_join(&a->mirrors, b.mirrors);
}

#endif /* __CLUSTERING_MIRROR_METADATA_HPP__ */
