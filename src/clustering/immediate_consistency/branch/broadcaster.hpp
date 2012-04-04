#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BROADCASTER_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BROADCASTER_HPP_

#include "errors.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "clustering/registrar.hpp"
#include "containers/uuid.hpp"
#include "protocol_api.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/semilattice/view.hpp"
#include "rpc/semilattice/view/field.hpp"
#include "rpc/semilattice/view/member.hpp"
#include "utils.hpp"
#include "timestamps.hpp"

/* Forward declarations (so we can friend them) */

template<class protocol_t> class listener_t;

/* The implementation of `broadcaster_t` is a mess, but the interface is
very clean. */

template<class protocol_t>
class broadcaster_t : public home_thread_mixin_t {
public:
    broadcaster_t(
            mailbox_manager_t *mm,
            boost::shared_ptr<semilattice_readwrite_view_t<branch_history_t<protocol_t> > > branch_history,
            store_view_t<protocol_t> *initial_store,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t) :
        mailbox_manager(mm),
        branch_id(generate_uuid()),
        registrar(mailbox_manager, this)
    {
        /* Snapshot the starting point of the store; we'll need to record this
        and store it in the metadata. */
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> read_token;
        initial_store->new_read_token(read_token);

        region_map_t<protocol_t, version_range_t> origins = 
            region_map_transform<protocol_t, binary_blob_t, version_range_t>(
                initial_store->get_metainfo(read_token, interruptor),
                &binary_blob_t::get<version_range_t>
            );

        /* Determine what the first timestamp of the new branch will be */
        state_timestamp_t initial_timestamp = state_timestamp_t::zero();

        typedef region_map_t<protocol_t, version_range_t> version_map_t;

        for (typename version_map_t::const_iterator it =  origins.begin();
                                                    it != origins.end();
                                                    it++) {
            state_timestamp_t part_timestamp = it->second.latest.timestamp;
            if (part_timestamp > initial_timestamp) {
                initial_timestamp = part_timestamp;
            }
        }
        current_timestamp = newest_complete_timestamp = initial_timestamp;

        /* Make an entry for this branch in the global branch history
        semilattice */
        {
            branch_birth_certificate_t<protocol_t> our_metadata;
            our_metadata.region = initial_store->get_region();
            our_metadata.initial_timestamp = initial_timestamp;
            our_metadata.origin = origins;

            std::map<branch_id_t, branch_birth_certificate_t<protocol_t> > singleton;
            singleton[branch_id] = our_metadata;
            metadata_field(&branch_history_t<protocol_t>::branches, branch_history)->join(singleton);
        }

        /* Reset the store metadata. We should do this after making the branch
        entry in the global metadata so that we aren't left in a state where
        the store has been marked as belonging to a branch for which no
        information exists. */
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> write_token;
        initial_store->new_write_token(write_token);
        initial_store->set_metainfo(
            region_map_t<protocol_t, binary_blob_t>(
                initial_store->get_region(),
                binary_blob_t(version_range_t(version_t(branch_id, initial_timestamp)))
            ),
            write_token,
            interruptor
            );

        /* Perform an initial sanity check. */
        sanity_check();

        /* Set `bootstrap_store` so that the initial listener can find it */
        bootstrap_store = initial_store;
    }

    /* TODO: These exceptions ought to be a bit more specific. Either there
    should be more exception classes or each one should give more information.
    */

    class mirror_lost_exc_t : public std::exception {
    public:
        const char *what() const throw () {
            return "We lost contact with the mirror. Maybe the mirror was "
                "destroyed, maybe the network went down, or maybe the "
                "`broadcaster_t` is being destroyed. The operation may "
                "or may not have been performed. You might be able to re-try "
                "the operation.";
        }
    };

    class insufficient_mirrors_exc_t : public std::exception {
    public:
        const char *what() const throw () {
            return "There are insufficient mirrors. This could be because the "
                "requested replication factor is greater than the number of "
                "mirrors, or because there aren't any readable mirrors. The "
                "operation may or may not have been completed.";
        }
    };

    // lock shall be reset by this function _before_ auto_drainer_lock is reset.
    typename protocol_t::read_response_t read(typename protocol_t::read_t r, fifo_enforcer_sink_t::exit_read_t *lock, auto_drainer_t::lock_t *auto_drainer_lock, order_token_t tok) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t);

    // lock shall be reset by this function _before_ auto_drainer_lock is reset.
    typename protocol_t::write_response_t write(typename protocol_t::write_t w, fifo_enforcer_sink_t::exit_write_t *lock, auto_drainer_t::lock_t *auto_drainer_lock, order_token_t tok) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t);

    branch_id_t get_branch_id() {
        return branch_id;
    }

    broadcaster_business_card_t<protocol_t> get_business_card() {
        return broadcaster_business_card_t<protocol_t>(
            branch_id, registrar.get_business_card());
    }

private:
    friend class listener_t<protocol_t>;

    class incomplete_write_ref_t;

    /* `incomplete_write_t` represents a write that has been sent to some nodes
    but not completed yet. */

    class incomplete_write_t : public home_thread_mixin_t {
    public:
        incomplete_write_t(broadcaster_t *p,
                typename protocol_t::write_t w, transition_timestamp_t ts,
                int target_ack_count_) :
            write(w), timestamp(ts),
            parent(p), incomplete_count(0),
            ack_count(0), target_ack_count(target_ack_count_)
        {
            rassert(target_ack_count > 0);
        }

        void notify_acked() {
            ack_count++;
            if (ack_count == target_ack_count) {
                done_promise.pulse(true);
            }
        }

        void notify_no_more_acks() {
            if (ack_count < target_ack_count) {
                done_promise.pulse(false);
            }
        }

        typename protocol_t::write_t write;
        transition_timestamp_t timestamp;

        /* `done_promise` gets pulsed with `true` when `target_ack_count` is
        reached, or pulsed with `false` if it will never be reached. */
        promise_t<bool> done_promise;

    private:
        friend class incomplete_write_ref_t;

        broadcaster_t *parent;
        int incomplete_count;
        int ack_count, target_ack_count;
    };

    /* We keep track of which `incomplete_write_t`s have been acked by all the
    nodes using `incomplete_write_ref_t`. When there are zero
    `incomplete_write_ref_t`s for a given `incomplete_write_t`, then it is no
    longer incomplete. */

    class incomplete_write_ref_t {
    public:
        incomplete_write_ref_t() { }
        explicit incomplete_write_ref_t(const boost::shared_ptr<incomplete_write_t> &w) : write(w) {
            rassert(w);
            w->incomplete_count++;
        }
        incomplete_write_ref_t(const incomplete_write_ref_t &r) : write(r.write) {
            if (r.write) {
                r.write->incomplete_count++;
            }
        }
        ~incomplete_write_ref_t() {
            if (write) {
                write->incomplete_count--;
                if (write->incomplete_count == 0) {
                    write->parent->end_write(write);
                }
            }
        }
        incomplete_write_ref_t &operator=(const incomplete_write_ref_t &r) {
            if (r.write) {
                r.write->incomplete_count++;
            }
            if (write) {
                write->incomplete_count--;
                if (write->incomplete_count == 0) {
                    write->parent->end_write(write);
                }
            }
            write = r.write;
            return *this;
        }
        boost::shared_ptr<incomplete_write_t> get() {
            return write;
        }
    private:
        boost::shared_ptr<incomplete_write_t> write;
    };

    /* The `registrar_t` constructs a `dispatchee_t` for every mirror that
    connects to us. */

    class dispatchee_t : public intrusive_list_node_t<dispatchee_t> {
    public:
        dispatchee_t(broadcaster_t *c, listener_business_card_t<protocol_t> d) THROWS_NOTHING;
        ~dispatchee_t() THROWS_NOTHING;

        typename listener_business_card_t<protocol_t>::write_mailbox_t::address_t write_mailbox;
        bool is_readable;
        typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t writeread_mailbox;
        typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t read_mailbox;

        /* This is used to enforce that operations are performed on the
        destination machine in the same order that we send them, even if the
        network layer reorders the messages. */
        fifo_enforcer_source_t fifo_source;

    private:
        /* The constructor spawns `send_intro()` in the background. */
        void send_intro(
            listener_business_card_t<protocol_t> to_send_intro_to,
            state_timestamp_t intro_timestamp,
            auto_drainer_t::lock_t)
            THROWS_NOTHING;

        /* `upgrade()` and `downgrade()` are mailbox callbacks. */
        void upgrade(
            typename listener_business_card_t<protocol_t>::writeread_mailbox_t::address_t,
            typename listener_business_card_t<protocol_t>::read_mailbox_t::address_t,
            auto_drainer_t::lock_t)
            THROWS_NOTHING;
        void downgrade(mailbox_addr_t<void()>, auto_drainer_t::lock_t) THROWS_NOTHING;

        broadcaster_t *controller;
        auto_drainer_t drainer;

        typename listener_business_card_t<protocol_t>::upgrade_mailbox_t upgrade_mailbox;
        typename listener_business_card_t<protocol_t>::downgrade_mailbox_t downgrade_mailbox;
    };

    /* Reads need to pick a single readable mirror to perform the operation.
    Writes need to choose a readable mirror to get the reply from. Both use
    `pick_a_readable_dispatchee()` to do the picking. You must hold
    `dispatchee_mutex` and pass in `proof` of the mutex acquisition. (A
    dispatchee is "readable" if a `replier_t` exists for it on the remote
    machine.) */
    void pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, mutex_t::acq_t *proof, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(insufficient_mirrors_exc_t);

    void background_write(dispatchee_t *, auto_drainer_t::lock_t, incomplete_write_ref_t, fifo_enforcer_write_token_t) THROWS_NOTHING;
    void end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING;

    /* This function sanity-checks `incomplete_writes`, `current_timestamp`,
    and `newest_complete_timestamp`. It mostly exists as a form of executable
    documentation. */
    void sanity_check() {
#ifndef NDEBUG
        mutex_t::acq_t acq(&mutex);
        state_timestamp_t ts = newest_complete_timestamp;
        for (typename std::list<boost::shared_ptr<incomplete_write_t> >::iterator it = incomplete_writes.begin();
                it != incomplete_writes.end(); it++) {
            rassert(ts == (*it)->timestamp.timestamp_before());
            ts = (*it)->timestamp.timestamp_after();
        }
        rassert(ts == current_timestamp);
#endif
    }

    mailbox_manager_t *mailbox_manager;

    branch_id_t branch_id;

    /* Until our initial listener has been constructed, this holds the store
    that was passed to our constructor. After that, it's `NULL`. */
    store_view_t<protocol_t> *bootstrap_store;

    /* If a write has begun, but some mirror might not have completed it yet,
    then it goes in `incomplete_writes`. The idea is that a new mirror that
    connects will use the union of a backfill and `incomplete_writes` as its
    data, and that will guarantee it gets at least one copy of every write.
    See the member function `sanity_check()` for a description of the
    relationship between `incomplete_writes`, `current_timestamp`, and
    `newest_complete_timestamp`. */

    /* `mutex` is held by new writes and reads being created, by writes
    finishing, and by dispatchees joining, leaving, or upgrading. It protects
    `incomplete_writes`, `current_timestamp`, `newest_complete_timestamp`,
    `order_sink`, `dispatchees`, and `readable_dispatchees`. */
    mutex_t mutex;

    std::list<boost::shared_ptr<incomplete_write_t> > incomplete_writes;
    state_timestamp_t current_timestamp, newest_complete_timestamp;
    order_sink_t order_sink;

    std::map<dispatchee_t *, auto_drainer_t::lock_t> dispatchees;
    intrusive_list_t<dispatchee_t> readable_dispatchees;

    registrar_t<listener_business_card_t<protocol_t>, broadcaster_t *, dispatchee_t> registrar;
};

#include "clustering/immediate_consistency/branch/broadcaster.tcc"

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_BROADCASTER_HPP_ */
