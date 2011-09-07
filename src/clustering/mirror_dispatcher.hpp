#ifndef __CLUSTERING_MIRROR_DISPATCHER_HPP__
#define __CLUSTERING_MIRROR_DISPATCHER_HPP__

#include "clustering/mirror_metadata.hpp"
#include "clustering/registrar.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/metadata/view.hpp"
#include "rpc/metadata/view/field.hpp"
#include "utils.hpp"
#include "timestamps.hpp"

/* The implementation of `mirror_dispatcher_t` is a mess, but the interface is
very clean. */

template<class protocol_t>
class mirror_dispatcher_t : public home_thread_mixin_t {

public:
    mirror_dispatcher_t(
            mailbox_cluster_t *c,
            metadata_readwrite_view_t<mirror_dispatcher_metadata_t<protocol_t> > *metadata,
            state_timestamp_t initial_timestamp
            ) THROWS_NOTHING :
        cluster(c),
        current_state_timestamp(initial_timestamp),
        registrar_view(&mirror_dispatcher_metadata_t<protocol_t>::registrar, metadata),
        registrar(cluster, this, &registrar_view)
        { }

    /* TODO: These exceptions ought to be a bit more specific. Either there
    should be more exception classes or each one should give more information.
    */

    class mirror_lost_exc_t : public std::exception {
    public:
        const char *what() const throw () {
            return "We lost contact with the mirror. Maybe the mirror was "
                "destroyed, maybe the network went down, or maybe the "
                "`mirror_dispatcher_t` is being destroyed. The operation may "
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

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, order_token_t tok) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t);

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, order_token_t tok) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t);

private:
    /* `incomplete_write_t` represents a write that has been sent to some nodes
    but not completed yet. */

    class incomplete_write_t :
        public home_thread_mixin_t
    {
    public:
        incomplete_write_t(mirror_dispatcher_t *p,
                typename protocol_t::write_t w, order_token_t otok,
                int target_ack_count_) :
            write(w), order_token(otok),
            parent(p),
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
        order_token_t order_token;

    private:
        mirror_dispatcher_t *parent;

        int incomplete_count;

        int ack_count, target_ack_count;

        /* `done_promise` gets pulsed with `true` when `target_ack_count` is
        reached, or pulsed with `false` if it will never be reached. */
        promise_t<bool> done_promise;
    };

    /* We keep track of which `incomplete_write_t`s have been acked by all the
    nodes using `incomplete_write_ref_t`. When there are zero
    `incomplete_write_ref_t`s for a given `incomplete_write_t`, then it is no
    longer incomplete. */

    class incomplete_write_ref_t {
    public:
        incomplete_write_ref_t() : write(NULL) {
        }
        incomplete_write_ref_t(const boost::shared_ptr<incomplete_write_t> &w) : write(w) {
            rassert(w);
            w->incomplete_count++;
        }
        incomplete_write_ref_t(const incomplete_write_ref_t &r) : write(NULL) {
            *this = r;
        }
        ~incomplete_write_ref_t() {
            *this = incomplete_write_ref_t();
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

    typedef typename mirror_dispatcher_metadata_t<protocol_t>::mirror_data_t mirror_data_t;

    /* The `registrar_t` constructs a `dispatchee_t` for every mirror that
    connects to us. */

    class dispatchee_t : public intrusive_list_node_t<dispatchee_t> {

    public:
        dispatchee_t(mirror_dispatcher_t *c, mirror_data_t d) THROWS_NOTHING;
        ~dispatchee_t() THROWS_NOTHING;

        typename mirror_data_t::write_mailbox_t::address_t write_mailbox;
        bool is_readable;
        typename mirror_data_t::writeread_mailbox_t::address_t writeread_mailbox;
        typename mirror_data_t::read_mailbox_t::address_t read_mailbox;

    private:
        void upgrade(
            typename mirror_data_t::writeread_mailbox_t::address_t,
            typename mirror_data_t::read_mailbox_t::address_t,
            auto_drainer_t::lock_t);

        mirror_dispatcher_t *controller;
        auto_drainer_t drainer;

        typename mirror_data_t::upgrade_mailbox_t upgrade_mailbox;
    };

    /* Reads need to pick a single readable mirror to perform the operation.
    Writes need to choose a readable mirror to get the reply from. Both use
    `pick_a_readable_dispatchee()` to do the picking. */
    void pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(insufficient_mirrors_exc_t);

    void background_write(dispatchee_t *, auto_drainer_t::lock_t, incomplete_write_ref_t) THROWS_NOTHING;
    void end_write(boost::shared_ptr<incomplete_write_t> write) THROWS_NOTHING;

    /* This function sanity-checks `incomplete_writes`, `current_timestamp`,
    and `newest_complete_timestamp`. It mostly exists as a form of executable
    documentation. */
    void sanity_check(mutex_acquisition_t *mutex_acq) {
#ifndef NDEBUG
        mutex_acq->assert_is_holding(&operation_mutex);
        state_timestamp_t ts = newest_complete_timestamp;
        for (std::list<boost::shared_ptr<incomplete_write_t> >::iterator it = incomplete_writes.begin();
                it != incomplete_writes.end(); it++) {
            rassert(ts == w->timestamp.timestamp_before());
            ts = w->timestamp.timestamp_after();
        }
        rassert(ts == current_timestamp);
#endif
    }

    mailbox_cluster_t *cluster;

    /* This mutex is held when an operation is starting or a new dispatchee is
    connecting. It protects `current_state_timestamp`,
    `newest_complete_timestamp`, `incomplete_writes`, `dispatchees`, and
    `order_checkpoint`. */
    mutex_t operation_mutex;

    order_checkpoint_t order_checkpoint;

    /* If a write has begun, but some mirror might not have completed it yet,
    then it goes in `incomplete_writes`. The idea is that a new mirror that
    connects will use the union of a backfill and `incomplete_writes` as its
    data, and that will guarantee it gets at least one copy of every write. */

    std::list<boost::shared_ptr<incomplete_write_t> > incomplete_writes;
    state_timestamp_t current_timestamp, newest_complete_timestamp;

    std::map<dispatchee_t *, auto_drainer_t::lock_t> dispatchees;

    intrusive_list_t<dispatchee_t> readable_dispatchees;

    metadata_field_readwrite_view_t<
        mirror_dispatcher_metadata_t<protocol_t>,
        resource_metadata_t<registrar_metadata_t<mirror_data_t> >
        > registrar_view;
    registrar_t<mirror_data_t, mirror_dispatcher_t *, dispatchee_t> registrar;
};

#include "clustering/mirror_dispatcher.tcc"

#endif /* __CLUSTERING_MIRROR_DISPATCHER_HPP__ */
