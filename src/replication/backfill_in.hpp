#ifndef __REPLICATION_BACKFILL_IN_HPP__
#define __REPLICATION_BACKFILL_IN_HPP__

#include "server/key_value_store.hpp"
#include "replication/backfill.hpp"
#include "concurrency/queue/limited_fifo.hpp"
#include "concurrency/coro_pool.hpp"

namespace replication {

/* A passive_producer_t that reads from a series of queues. When the
first one is empty, it tries the next one. */

template <class value_t>
class selective_passive_producer_t : public passive_producer_t<value_t> {
public:
    selective_passive_producer_t(passive_producer_t<value_t> *selectee) :
        passive_producer_t<value_t>(&available_control),
        the_producer(NULL) {
        set_source(selectee);
    }
    ~selective_passive_producer_t() {
        set_source(NULL);
    }
    void set_source(passive_producer_t<value_t> *selectee) {
        if (the_producer) {
            the_producer->available->unset_callback();
            the_producer = NULL;
        }
        if (selectee != NULL) {
            the_producer = selectee;
            the_producer->available->set_callback(boost::bind(&selective_passive_producer_t<value_t>::recompute, this));
        }
        recompute();
    }

private:

    void recompute() {
        /* We are available if our source is available. */
        available_control.set_available(the_producer && the_producer->available->get());
    }
    value_t produce_next_value() {
        rassert(available_control.get());
        rassert(the_producer, "available_control doesn't match the_producer");
        return the_producer->pop();
    }

    passive_producer_t<value_t> *the_producer;
    availability_control_t available_control;

    DISABLE_COPYING(selective_passive_producer_t);
};

/* backfill_storer_t is the dual of the backfill_and_realtime_stream() function. It takes
the updates that backfill_and_realtime_stream() extracts from a key-value store, and it puts
them into another key-value store.

Usually, they are transmitted over the network between when they are extracted by
backfill_and_realtime_stream() and when they are stored by backfill_storer_t. */

class backfill_storer_t : public backfill_and_realtime_streaming_callback_t {
public:
    backfill_storer_t(btree_key_value_store_t *underlying);
    ~backfill_storer_t();

    void backfill_delete_range(int hash_value, int hashmod, store_key_t low_key, store_key_t high_key, order_token_t token);
    void backfill_deletion(store_key_t key, order_token_t token);
    void backfill_set(backfill_atom_t atom, order_token_t token);
    void backfill_done(repli_timestamp_t timestamp, order_token_t token);

    void realtime_get_cas(const store_key_t& key, castime_t castime, order_token_t token);
    void realtime_sarc(sarc_mutation_t& m, castime_t castime, order_token_t token);
    void realtime_incr_decr(incr_decr_kind_t kind, const store_key_t& key, uint64_t amount,
                            castime_t castime, order_token_t token);
    void realtime_append_prepend(append_prepend_kind_t kind, const store_key_t& key,
                                 const boost::shared_ptr<data_provider_t>& data, castime_t castime, order_token_t token);
    void realtime_delete_key(const store_key_t& key, repli_timestamp_t timestamp, order_token_t token);
    void realtime_time_barrier(repli_timestamp_t timestamp, order_token_t token);

private:
    // Should be called whenever a backfilling operation is received. Makes sure
    // that no realtime operations are getting processed and that backfilling_ is
    // set to true
    void ensure_backfilling();

    btree_key_value_store_t *kvs_;
    bool backfilling_, print_backfill_warning_;
    limited_fifo_queue_t<boost::function<void()> > backfill_queue_, realtime_queue_;
    selective_passive_producer_t<boost::function<void()> > queue_picker_;
    coro_pool_t coro_pool_;
};

}

#endif /* __REPLICATION_BACKFILL_IN_HPP__ */
