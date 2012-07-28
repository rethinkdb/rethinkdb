#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP_

#include <map>
#include <utility>

#include "clustering/generic/registration_metadata.hpp"
#include "clustering/generic/resource.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/promise.hpp"
#include "containers/printf_buffer.hpp"
#include "containers/uuid.hpp"
#include "protocol_api.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/joins/map.hpp"
#include "timestamps.hpp"

/* Every broadcaster generates a UUID when it's first created. This is the UUID
of the branch that the broadcaster administers. */

typedef uuid_t branch_id_t;

/* `version_t` is a (branch ID, timestamp) pair. A `version_t` uniquely
identifies the state of some region of the database at some time. */

class version_t {
public:
    version_t() { }
    version_t(branch_id_t bid, state_timestamp_t ts) :
        branch(bid), timestamp(ts) { }
    static version_t zero() {
        return version_t(nil_uuid(), state_timestamp_t::zero());
    }

    bool operator==(const version_t &v) const{
        return branch == v.branch && timestamp == v.timestamp;
    }
    bool operator!=(const version_t &v) const {
        return !(*this == v);
    }

    branch_id_t branch;
    state_timestamp_t timestamp;

    RDB_MAKE_ME_SERIALIZABLE_2(branch, timestamp);
};

inline void debug_print(append_only_printf_buffer_t *buf, const version_t& v) {
    buf->appendf("v{");
    debug_print(buf, v.branch);
    buf->appendf(", ");
    debug_print(buf, v.timestamp);
    buf->appendf("}");
}

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

    bool is_coherent() const {
        return earliest == latest;
    }
    bool operator==(const version_range_t &v) const {
        return earliest == v.earliest && latest == v.latest;
    }
    bool operator!=(const version_range_t &v) const {
        return !(*this == v);
    }

    version_t earliest, latest;

    RDB_MAKE_ME_SERIALIZABLE_2(earliest, latest);
};

inline void debug_print(append_only_printf_buffer_t *buf, const version_range_t& vr) {
    buf->appendf("vr{earliest=");
    debug_print(buf, vr.earliest);
    buf->appendf(", latest=");
    debug_print(buf, vr.latest);
    buf->appendf("}");
}

/* `branch_history_t` is a record of some set of branches. They are stored on
disk and passed back and forth between machines in such a way that each machine
always has history for every branch that it has to work with. */

template<class protocol_t>
class branch_birth_certificate_t {
public:
    /* The region covered by the branch */
    typename protocol_t::region_t region;

    /* The timestamp of the first state on the branch */
    state_timestamp_t initial_timestamp;

    /* Where the branch's initial data came from */
    region_map_t<protocol_t, version_range_t> origin;

    RDB_MAKE_ME_SERIALIZABLE_3(region, initial_timestamp, origin);
};

template<class protocol_t>
class branch_history_t {
public:
    std::map<branch_id_t, branch_birth_certificate_t<protocol_t> > branches;

    RDB_MAKE_ME_SERIALIZABLE_1(branches);
};

/* Every `listener_t` constructs a `listener_business_card_t` and sends it to
the `broadcaster_t`. */

template<class protocol_t>
class listener_business_card_t {

public:
    /* These are the types of mailboxes that the master uses to communicate with
    the mirrors. */

    typedef mailbox_t<void(typename protocol_t::write_t,
                            transition_timestamp_t,
                            order_token_t,
                            fifo_enforcer_write_token_t,
                            mailbox_addr_t<void()>)> write_mailbox_t;

    typedef mailbox_t<void(typename protocol_t::write_t,
                           transition_timestamp_t,
                           order_token_t,
                           fifo_enforcer_write_token_t,
                           mailbox_addr_t<void(typename protocol_t::write_response_t)>)> writeread_mailbox_t;

    typedef mailbox_t<void(typename protocol_t::read_t,
                           state_timestamp_t,
                           order_token_t,
                           fifo_enforcer_read_token_t,
                           mailbox_addr_t<void(typename protocol_t::read_response_t)>)> read_mailbox_t;

    /* The master sends a single message to `intro_mailbox` at the very
    beginning. This tells the mirror what timestamp it's at, and also tells
    it where to send upgrade/downgrade messages. */

    typedef mailbox_t<void(typename writeread_mailbox_t::address_t,
                           typename read_mailbox_t::address_t)> upgrade_mailbox_t;

    typedef mailbox_t<void(mailbox_addr_t<void()>)> downgrade_mailbox_t;

    typedef mailbox_t<void(state_timestamp_t,
                           typename upgrade_mailbox_t::address_t,
                           typename downgrade_mailbox_t::address_t)> intro_mailbox_t;

    listener_business_card_t() { }
    listener_business_card_t(const typename intro_mailbox_t::address_t &im,
                             const typename write_mailbox_t::address_t &wm)
        : intro_mailbox(im), write_mailbox(wm) { }

    typename intro_mailbox_t::address_t intro_mailbox;
    typename write_mailbox_t::address_t write_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_2(intro_mailbox, write_mailbox);
};

/* `backfiller_business_card_t` represents a thing that is willing to serve
backfills over the network. It appears in the directory. */

typedef uuid_t backfill_session_id_t;

template<class protocol_t>
struct backfiller_business_card_t {

    typedef mailbox_t< void(
        backfill_session_id_t,
        region_map_t<protocol_t, version_range_t>,
        branch_history_t<protocol_t>,
        mailbox_addr_t< void(
            region_map_t<protocol_t, version_range_t>,
            branch_history_t<protocol_t>
            ) >,
        mailbox_addr_t<void(typename protocol_t::backfill_chunk_t, fifo_enforcer_write_token_t)>,
        mailbox_t<void(fifo_enforcer_write_token_t)>::address_t,
        mailbox_t<void(mailbox_addr_t<void(int)>)>::address_t
        )> backfill_mailbox_t;

    typedef mailbox_t<void(backfill_session_id_t)> cancel_backfill_mailbox_t;


    /* Mailboxes used for requesting the progress of a backfill */
    typedef mailbox_t<void(backfill_session_id_t, mailbox_addr_t<void(std::pair<int, int>)>)> request_progress_mailbox_t;

    backfiller_business_card_t() { }
    backfiller_business_card_t(
            const typename backfill_mailbox_t::address_t &ba,
            const cancel_backfill_mailbox_t::address_t &cba,
            const request_progress_mailbox_t::address_t &pa) :
        backfill_mailbox(ba), cancel_backfill_mailbox(cba), request_progress_mailbox(pa)
        { }

    typename backfill_mailbox_t::address_t backfill_mailbox;
    cancel_backfill_mailbox_t::address_t cancel_backfill_mailbox;
    request_progress_mailbox_t::address_t request_progress_mailbox;

    RDB_MAKE_ME_SERIALIZABLE_3(backfill_mailbox, cancel_backfill_mailbox, request_progress_mailbox);
};

/* `broadcaster_business_card_t` is the way that listeners find the broadcaster.
It appears in the directory. */

template<class protocol_t>
struct broadcaster_business_card_t {

    broadcaster_business_card_t(branch_id_t bid,
            const branch_history_t<protocol_t> &bh,
            const registrar_business_card_t<listener_business_card_t<protocol_t> > &r) :
        branch_id(bid), branch_id_associated_branch_history(bh), registrar(r) { }

    broadcaster_business_card_t() { }

    branch_id_t branch_id;
    branch_history_t<protocol_t> branch_id_associated_branch_history;

    registrar_business_card_t<listener_business_card_t<protocol_t> > registrar;

    RDB_MAKE_ME_SERIALIZABLE_3(branch_id, branch_id_associated_branch_history, registrar);
};

template<class protocol_t>
struct replier_business_card_t {
    /* This mailbox is used to check that the replier is at least as up to date
     * as the timestamp. The second argument is used as an ack mailbox, once
     * synchronization is complete the replier will send a message to it. */
    typedef mailbox_t<void(state_timestamp_t, mailbox_addr_t<void()>)> synchronize_mailbox_t;
    synchronize_mailbox_t::address_t synchronize_mailbox;

    backfiller_business_card_t<protocol_t> backfiller_bcard;

    replier_business_card_t()
    { }

    replier_business_card_t(const synchronize_mailbox_t::address_t &_synchronize_mailbox, const backfiller_business_card_t<protocol_t> &_backfiller_bcard)
        : synchronize_mailbox(_synchronize_mailbox), backfiller_bcard(_backfiller_bcard)
    { }

    RDB_MAKE_ME_SERIALIZABLE_2(synchronize_mailbox, backfiller_bcard);
};

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_METADATA_HPP_ */
