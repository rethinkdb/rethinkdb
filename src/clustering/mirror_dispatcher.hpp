#ifndef __CLUSTERING_MIRROR_DISPATCHER_HPP__
#define __CLUSTERING_MIRROR_DISPATCHER_HPP__

/* The implementation of this module is a mess, but the interface is very clean.
*/

template<class protocol_t>
class mirror_dispatcher_t {

public:
    mirror_dispatcher_t(
            mailbox_cluster_t *c,
            metadata_readwrite_view_t<mirror_dispatcher_metadata_t<protocol_t> > *metadata
            ) THROWS_NOTHING :
        cluster(c),
        registrar_view(&mirror_dispatcher_metadata_t<protocol_t>::registrar, metadata),
        registrar(cluster, this, &registrar_view)
        { }

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
    }

    typename protocol_t::read_response_t read(typename protocol_t::read_t r, order_token_t tok) THROWS_ONLY(mirror_lost_exc_t, insufficient_mirrors_exc_t) {

        dispatchee_t *reader;
        auto_drainer_t::lock_t reader_lock;
        pick_a_readable_dispatchee(&reader, &reader_lock);
        return reader->read(r, tok, reader_lock);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t w, repli_timestamp_t ts, order_token_t tok) {

        /* `write_ref` is to keep `write` from freeing itself before we're done
        with it */
        queued_write_t::ref_t write_ref;
        queued_write_t *write;

        dispatchee_t *writereader;
        auto_drainer_t::lock_t writereader_lock;

        /* TODO: Make `target_ack_count` configurable */
        int target_ack_count = 1;
        promise_t<bool> done_promise;

        {
            /* We want to make sure that we send the write to every dispatchee
            and put it into the write queue atomically, so that every dispatchee
            either gets it directly or gets it off the write queue. Putting a
            critical section here makes sure that everything but the call to
            `writeread()` gets done atomically. Unfortunately, there's nothing
            we can do to enforce that nothing else happens between the insertion
            into `write_queue` and the call to `writeread`. */
            ASSERT_FINITE_CORO_WAITING;

            pick_a_readable_dispatchee(&writereader, &writereader_lock);

            write = new queued_write_t(
                &write_queue, &write_ref,
                w, ts, tok,
                target_ack_count, &done_promise);

            for (std::map<dispatchee_t *, auto_drainer_t::lock_t>::iterator it = dispatchees.begin();
                    it != dispatchees.end(); it++) {
                dispatchee_t *writer = (*it).first;
                auto_drainer_t::lock_t writer_lock = (*it).second;
                /* Make sure not to send a duplicate operation to `writereader`
                */
                if (writer != writereader) {
                    writer->begin_write_in_background(write, queued_write_t::ref_t(write), writer_lock);
                }
            }
        }

        typename protocol_t::write_response_t resp = writereader->writeread(write, queued_write_t::ref_t(write), writereader_lock);

        /* Release our reference to `write` so that it can go away if everything
        else is done with it too. */
        write_ref = queued_write_t::ref_t();

        bool success = done_promise.wait();
        if (!success) throw insufficient_mirrors_exc_t();

        return resp;
    }

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
            ~ref_t() {
                if (parent) parent->decref();
            }
        private:
            queued_write_t *parent;
        };

        queued_write_t(intrusive_list_t<queued_write_t> *q, ref_t *first_ref_out,
                typename protocol_t::write_t w, repli_timestamp_t ts, order_token_t otok,
                int target_ack_count_, promise_t<bool> *done_promise_) :
            queue(q), write(w), timestamp(ts), order_token(otok),
            ack_count(0), target_ack_count(target_ack_count_), done_promise(done_promise_),
            ref_count(0)
        {
            ASSERT_FINITE_CORO_WAITING;
            queue->push_back(this);
            *first_ref_out = ref_t(this);
        }

        ~queued_write_t() {
            queue->remove(this);
            if (done_promise) done_promise->pulse(false);
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

        int ack_count, target_ack_count;
        promise_t<bool> *done_promise;

        /* While `ref_count` is greater than zero, the write must remain in the
        `write_queue` so that any new mirrors that come up will be sure to get
        it. */
        int ref_count;
    };

    /* The `registrar_t` constructs a `dispatchee_t` for every mirror that
    connects to us. */

    class dispatchee_t : public intrusive_list_node_t<dispatchee_t> {

    public:
        dispatchee_t(mirror_dispatcher_t *c, mirror_data_t d) THROWS_NOTHING :
            controller(c), is_readable(false)
        {
            ASSERT_FINITE_CORO_WAITING;

            controller->dispatchees[this] = auto_drainer_t::lock_t(&drainer);
            update(d);

            for (queued_write_t *write = controller->write_queue.tail();
                    write; write = controller->write_queue.next(write)) {
                begin_write_in_background(write, queued_write_t::ref_t(write),
                    auto_drainer_t::lock_t(&drainer));
            }
        }

        ~dispatchee_t() THROWS_NOTHING {
            if (is_readable) controller->readable_dispatchees.remove(this);
            controller->dispatchees.erase(this);
        }

        void update(mirror_data_t d) THROWS_NOTHING {
            data = d;
            if (is_readable) {
                /* It's illegal to become unreadable after becoming readable */
                rassert(!data.writeread_mailbox.is_nil());
                rassert(!data.read_mailbox.is_nil());
            } else if (!is_readable && !d.read_mailbox.is_nil()) {
                /* We're upgrading to be readable */
                rassert(!d.writeread_mailbox.is_nil());
                is_readable = true;
                controller->readable_dispatchees.push_back(this);
            }
        }

        typename protocol_t::write_response_t writeread(queued_write_t *write, queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive) THROWS_ONLY(mirror_lost_exc_t) {
            typename protocol_t::write_response_t resp;
            try {
                resp = mirror_data_writeread(controller->cluster, data,
                    write->write, write->timestamp, write->order_token,
                    keepalive.get_drain_signal());
            } catch (interrupted_exc_t) {
                throw mirror_lost_exc_t();
            }
            /* TODO: Inform `write` that it's safely on a mirror */
            return resp;
        }

        typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t order_token, auto_drainer_t::lock_t keepalive) THROWS_ONLY(mirror_lost_exc_t) {
            try {
                return mirror_data_read(controller->cluster, data,
                    read, order_token,
                    keepalive.get_drain_signal());
            } catch (interrupted_exc_t) {
                throw mirror_lost_exc_t();
            }
        }

        void begin_write_in_background(queued_write_t *write, queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive) THROWS_NOTHING {
            /* It's safe to allocate this directly on the heap because
            `coro_t::spawn_sometime()` should always succeed, and
            `write_in_background()` will free `our_place_in_line`. */
            coro_fifo_acq_t *our_place_in_line = new coro_fifo_acq_t(&background_write_fifo);
            coro_t::spawn_sometime(boost::bind(&dispatchee_t::write_in_background, write, write_ref, keepalive, our_place_in_line));
        }

    private:
        void write_in_background(queued_write_t *write, queued_write_t::ref_t write_ref, auto_drainer_t::lock_t keepalive, coro_fifo_acq_t *our_place_in_line) THROWS_NOTHING {
            delete spot_in_line;
            try {
                mirror_data_write(controller->cluster, data,
                    write->write, write->timestamp, write->order_token,
                    keepalive.get_drain_signal());
            } catch (interrupted_exc_t) {
                return;
            }
            /* TODO: Inform `write` that it's safely on a mirror */
        }

        mirror_dispatcher_t *controller;

        mirror_data_t data;
        bool is_readable;

        coro_fifo_t background_write_fifo;

        auto_drainer_t drainer;
    };

    /* Reads need to pick a single readable mirror to perform the operation.
    Writes need to choose a readable mirror to get the reply from. Both use
    `pick_a_readable_dispatchee()` to do the picking. */
    void pick_a_readable_dispatchee(dispatchee_t **dispatchee_out, auto_drainer_t::lock_t *lock_out) THROWS_ONLY(insufficient_mirrors_exc_t) {

        ASSERT_FINITE_CORO_WAITING;

        if (readable_dispatchees.empty()) throw insufficient_mirrors_exc_t();
        *dispatchee_out = readable_dispatchees.head();

        /* Cycle the readable dispatchees so that the load gets distributed
        evenly */
        readable_dispatchees.pop_front();
        readable_dispatchees.push_back(*dispatchee_out);

        *lock_out = dispatchees[*dispatchee_out];
    }

    mailbox_cluster_t *cluster;

    intrusive_list_t<queued_write_t> write_queue;

    std::map<dispatchee_t *, auto_drainer_t::lock_t> dispatchees;
    intrusive_list_t<dispatchee_t> readable_dispatchees;

    metadata_field_readwrite_view_t<
        mirror_dispatcher_metadata_t<protocol_t>,
        resource_metadata_t<registrar_metadata_t<
            mirror_metadata_t<protocol_t>::mirror_data_t
            > >
        > registrar_view;
    registrar_t<
        mirror_metadata_t<protocol_t>::mirror_data_t,
        mirror_dispatcher_t *,
        dispatchee_t
        > registrar;
};

#endif /* __CLUSTERING_MIRROR_DISPATCHER_HPP__ */
