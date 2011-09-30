#ifndef __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP__
#define __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP__

#include <map>

#include "errors.hpp"
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/variant.hpp>

#include "clustering/registration_metadata.hpp"
#include "clustering/resource.hpp"
#include "concurrency/fifo_checker.hpp"
#include "protocol_api.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/metadata/semilattice/map.hpp"
#include "timestamps.hpp"

/* Objects that have a presence in the metadata are identified by UUID */

typedef boost::uuids::uuid backfiller_id_t;

typedef boost::uuids::uuid branch_id_t;

/* `version_t` is a (branch ID, timestamp) pair. A `version_t` uniquely
identifies the state of some region of the database at some time. */

class version_t {
public:
    version_t() { }
    version_t(branch_id_t bid, state_timestamp_t ts) :
        branch(bid), timestamp(ts) { }

    bool operator==(const version_t &v) {
        return branch == v.branch && timestamp == v.timestamp;
    }
    bool operator!=(const version_t &v) {
        return !(*this == v);
    }

    branch_id_t branch;
    state_timestamp_t timestamp;

    RDB_MAKE_ME_SERIALIZABLE_2(branch, timestamp);
};

/* `version_range_t` is a pair of `version_t`s. It's used to keep track of
backfills; when a backfill is interrupted, the state of the individual keys is
unknown and all we know is that they lie within some range of versions. */

class version_range_t {
public:
    version_range_t() { }
    explicit version_range_t(const version_t &v) :
        earliest(v), latest(v) { }
    version_range_t(const version_t &e, const version_t &l) :
        earliest(e), latest(l) { }

    bool is_coherent() {
        return earliest == latest;
    }
    bool operator==(const version_range_t &v) {
        return earliest == v.earliest && latest == v.latest;
    }
    bool operator!=(const version_range_t &v) {
        return !(*this == v);
    }

    version_t earliest, latest;

    RDB_MAKE_ME_SERIALIZABLE_2(earliest, latest);
};

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

    listener_data_t() { }
    listener_data_t(
            const typename intro_mailbox_t::address_t &im,
            const typename write_mailbox_t::address_t &wm) :
        intro_mailbox(im), write_mailbox(wm) { }

    typename intro_mailbox_t::address_t intro_mailbox;
    typename write_mailbox_t::address_t write_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_2(intro_mailbox, write_mailbox);
};

/* `backfiller_metadata_t` represents a thing that is willing to serve backfills
over the network. */

template<class protocol_t>
struct backfiller_metadata_t {

    typedef boost::uuids::uuid backfill_session_id_t;

    typedef async_mailbox_t<void(
        backfill_session_id_t,
        region_map_t<protocol_t, version_range_t>,
        typename async_mailbox_t<void(region_map_t<protocol_t, version_range_t>)>::address_t,
        typename async_mailbox_t<void(typename protocol_t::backfill_chunk_t)>::address_t,
        async_mailbox_t<void()>::address_t
        )> backfill_mailbox_t;

    typedef async_mailbox_t<void(
        backfill_session_id_t
        )> cancel_backfill_mailbox_t;

    backfiller_metadata_t() { }
    backfiller_metadata_t(
            const typename backfill_mailbox_t::address_t &ba,
            const cancel_backfill_mailbox_t::address_t &cba) :
        backfill_mailbox(ba), cancel_backfill_mailbox(cba)
        { }

    typename backfill_mailbox_t::address_t backfill_mailbox;
    cancel_backfill_mailbox_t::address_t cancel_backfill_mailbox;
};

template<class protocol_t>
void semilattice_join(UNUSED backfiller_metadata_t<protocol_t> *a, UNUSED const backfiller_metadata_t<protocol_t> &b) {
    /* They should be equal, but we don't bother testing that */
}

/* `branch_metadata_t` is all the metadata for a branch. */

template<class protocol_t>
class branch_metadata_t {

public:
    /* History metadata for the branch. This never changes. */
    typename protocol_t::region_t region;
    state_timestamp_t initial_timestamp;
    region_map_t<protocol_t, version_range_t> origin;

    /* When listeners start up, they construct a `listener_data_t` and send it
    to the broadcaster via `broadcaster_registrar` */
    resource_metadata_t<registrar_metadata_t<listener_data_t<protocol_t> > > broadcaster_registrar;

    /* When repliers start up, they make entries in `backfillers` so new
    listeners can find them for backfills */
    std::map<backfiller_id_t, resource_metadata_t<backfiller_metadata_t<protocol_t> > > backfillers;

    RDB_MAKE_ME_SERIALIZABLE_5(region, initial_timestamp, origin,
        broadcaster_registrar, backfillers);
};

template<class protocol_t>
void semilattice_join(branch_metadata_t<protocol_t> *a, const branch_metadata_t<protocol_t> &b) {
    rassert(a->region == b.region);
    rassert(a->initial_timestamp == b.initial_timestamp);
    /* `origin` should be the same too, but don't bother comparing */
    semilattice_join(&a->broadcaster_registrar, b.broadcaster_registrar);
    semilattice_join(&a->backfillers, b.backfillers);
}

/* The namespace as a whole has a `namespace_branches_metadata_t` that holds all
of the branches. */

template<class protocol_t>
class namespace_branch_metadata_t {

public:
    std::map<branch_id_t, branch_metadata_t<protocol_t> > branches;

    RDB_MAKE_ME_SERIALIZABLE_1(branches);
};

template<class protocol_t>
void semilattice_join(namespace_branch_metadata_t<protocol_t> *a, const namespace_branch_metadata_t<protocol_t> &b) {
    semilattice_join(&a->branches, b.branches);
}

#endif /* __CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP__ */
