#ifndef __CLUSTERING_MIRROR_DISPATCHER_HPP__
#define __CLUSTERING_MIRROR_DISPATCHER_HPP__

#include "clustering/mirror_metadata.hpp"
#include "clustering/registrar.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/metadata/view.hpp"
#include "rpc/metadata/view/field.hpp"
#include "utils.hpp"

/* The implementation of `mirror_dispatcher_t` is a mess, but the interface is
very clean. */

template<class protocol_t>
class mirror_dispatcher_t : public home_thread_mixin_t {

public:
    mirror_dispatcher_t(
            mailbox_cluster_t *c,
            metadata_readwrite_view_t<mirror_dispatcher_metadata_t<protocol_t> > *metadata
            ) THROWS_NOTHING :
        cluster(c),
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

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, repli_timestamp_t ts, order_token_t tok) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t);

private:
    /* `queued_write_t` represents a write that has been sent to some nodes but
    not completed yet. In its constructor, it puts itself into `write_queue`; in
    its destructor, it removes itself. */

    class queued_write_t :
        public intrusive_list_node_t<queued_write_t>,
        public home_thread_mixin_t
    {
    public:
        class ref_t {
        public:
            ref_t() : parent(NULL) { }
            ref_t(queued_write_t *w) : parent(w) {
                parent->incref();
            }
            ref_t(const ref_t &r) : parent(r.parent) {
                parent->incref();
            }
            ref_t &operator=(const ref_t &r) {
                if (r.parent) r.parent->incref();
                if (parent) parent->decref();
                parent = r->parent;
            }
            queued_write_t *get() {
                rassert(parent);
                return parent;
            }
            ~ref_t() {
                if (parent) parent->decref();
            }
        private:
            queued_write_t *parent;
        };

        static ref_t spawn(intrusive_list_t<queued_write_t> *q,
                typename protocol_t::write_t w, repli_timestamp_t ts, order_token_t otok,
                int target_ack_count, promise_t<bool> *done_promise) THROWS_NOTHING {
            ASSERT_FINITE_CORO_WAITING;
            queued_write_t *write = new queued_write_t(q, w, ts, otok, target_ack_count, done_promise);
            return ref_t(write);
        }

        void notify_acked() {
            ack_count++;
            if (ack_count == target_ack_count) {
                done_promise->pulse(true);
                done_promise = NULL;
            }
        }

        typename protocol_t::write_t write;
        repli_timestamp_t timestamp;
        order_token_t order_token;

    private:
        queued_write_t(intrusive_list_t<queued_write_t> *q,
                typename protocol_t::write_t w, repli_timestamp_t ts, order_token_t otok,
                int target_ack_count_, promise_t<bool> *done_promise_) :
            write(w), timestamp(ts), order_token(otok),
            queue(q), ref_count(0),
            ack_count(0), target_ack_count(target_ack_count_), done_promise(done_promise_)
        {
            ASSERT_FINITE_CORO_WAITING;
            queue->push_back(this);
        }

        ~queued_write_t() {
            queue->remove(this);
            if (done_promise) done_promise->pulse(false);
        }

        void incref() {
            assert_thread();
            ref_count++;
        }

        void decref() {
            assert_thread();
            ref_count--;
            if (ref_count == 0) delete this;
        }

        intrusive_list_t<queued_write_t> *queue;

        /* While `ref_count` is greater than zero, the write must remain in the
        `write_queue` so that any new mirrors that come up will be sure to get
        it. */
        int ref_count;

        int ack_count, target_ack_count;
        promise_t<bool> *done_promise;
    };

    typedef typename mirror_dispatcher_metadata_t<protocol_t>::mirror_data_t mirror_data_t;

    /* The `registrar_t` constructs a `dispatchee_t` for every mirror that
    connects to us. */

    class dispatchee_t : public intrusive_list_node_t<dispatchee_t> {

    public:
        dispatchee_t(mirror_dispatcher_t *c, mirror_data_t d) THROWS_NOTHING;
        ~dispatchee_t() THROWS_NOTHING;
        void update(mirror_data_t d) THROWS_NOTHING;

        typename protocol_t::write_response_t writeread(typename queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive) THROWS_ONLY(mirror_lost_exc_t);
        typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t order_token, auto_drainer_t::lock_t keepalive) THROWS_ONLY(mirror_lost_exc_t);
        void begin_write_in_background(typename queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive) THROWS_NOTHING;

    private:
        void write_in_background(typename queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive, coro_fifo_acq_t *our_place_in_line) THROWS_NOTHING;

        mirror_dispatcher_t *controller;

        mirror_data_t data;
        bool is_readable;

        coro_fifo_t background_write_fifo;

        auto_drainer_t drainer;
    };

    /* Reads need to pick a single readable mirror to perform the operation.
    Writes need to choose a readable mirror to get the reply from. Both use
    `pick_a_readable_dispatchee()` to do the picking. */
    void pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(insufficient_mirrors_exc_t);

    mailbox_cluster_t *cluster;

    intrusive_list_t<queued_write_t> write_queue;

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
