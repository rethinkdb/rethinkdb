#ifndef __REPLICATION_BACKFILL_IN_HPP__
#define __REPLICATION_BACKFILL_IN_HPP__

#include "server/key_value_store.hpp"
#include "replication/backfill.hpp"
#include "concurrency/queue/limited_fifo.hpp"
#include "concurrency/coro_pool.hpp"

namespace replication {

/* `listing_passive_producer_t` is a `passive_producer_t` that reads from a
series of queues. When the first one is empty, it tries the next one. */

template<class value_t>
struct listing_passive_producer_t :
    public passive_producer_t<value_t>
{
    listing_passive_producer_t(std::vector<passive_producer_t<value_t> *> producers) :
        passive_producer_t<value_t>(&available_var)
    {
        set_sources(producers);
    }
    ~listing_passive_producer_t() {
        set_sources(std::vector<passive_producer_t<value_t> *>());
    }
    void set_sources(std::vector<passive_producer_t<value_t> *> producers) {
        while (watcher_t *w = watchers.head()) {
            watchers.remove(w);
            delete w;
        }
        for (int i = 0; i < (int)producers.size(); i++) {
            watcher_t *w = new watcher_t(this, producers[i]);
            watchers.push_back(w);
        }
        recompute();
    }

private:
    struct watcher_t :
        public watchable_t<bool>::watcher_t,
        public intrusive_list_node_t<watcher_t>
    {
        watcher_t(listing_passive_producer_t *par, passive_producer_t<value_t> *pro) :
            parent(par), producer(pro)
        {
            producer->available->add_watcher(this);
        }
        ~watcher_t() {
            producer->available->remove_watcher(this);
        }
        void on_watchable_changed() {
            parent->recompute();
        }
        listing_passive_producer_t *parent;
        passive_producer_t<value_t> *producer;
    };
    intrusive_list_t<watcher_t> watchers;

    void recompute() {
        /* We are available if any of our sources is available. */
        bool a = false;
        for (typename intrusive_list_t<watcher_t>::iterator it = watchers.begin();
                it != watchers.end(); it++) {
            a = a || (*it).producer->available->get();
        }
        available_var.set(a);
    }
    watchable_var_t<bool> available_var;
    value_t produce_next_value() {
        /* Find the first available source. */
        for (typename intrusive_list_t<watcher_t>::iterator it = watchers.begin();
                it != watchers.end(); it++) {
            if ((*it).producer->available->get()) {
                return (*it).producer->pop();
            }
        }
        /* If none of the producers were `available`, then our `available_var`
        should have been `false`, so `produce_next_value()` should never have
        been called. */
        unreachable();
    }
};

/* backfill_storer_t is the dual of the backfill_and_realtime_stream() function. It takes
the updates that backfill_and_realtime_stream() extracts from a key-value store, and it puts
them into another key-value store.

Usually, they are transmitted over the network between when they are extracted by
backfill_and_realtime_stream() and when they are stored by backfill_storer_t. */

struct backfill_storer_t : public backfill_and_realtime_streaming_callback_t {

    backfill_storer_t(btree_key_value_store_t *underlying);
    ~backfill_storer_t();

    void backfill_delete_everything(order_token_t token);
    void backfill_deletion(store_key_t key, order_token_t token);
    void backfill_set(backfill_atom_t atom, order_token_t token);
    void backfill_done(repli_timestamp_t timestamp, order_token_t token);

    void realtime_get_cas(const store_key_t& key, castime_t castime, order_token_t token);
    void realtime_sarc(sarc_mutation_t& m, castime_t castime, order_token_t token);
    void realtime_incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount,
                            castime_t castime, order_token_t token);
    void realtime_append_prepend(append_prepend_kind_t kind, const store_key_t &key,
                                 unique_ptr_t<data_provider_t> data, castime_t castime, order_token_t token);
    void realtime_delete_key(const store_key_t &key, repli_timestamp timestamp, order_token_t token);
    void realtime_time_barrier(repli_timestamp_t timestamp, order_token_t token);

private:
    btree_key_value_store_t *kvs_;
    bool print_backfill_warning_;
    limited_fifo_queue_t<boost::function<void()> > backfill_queue_, realtime_queue_;
    listing_passive_producer_t<boost::function<void()> > queue_picker_;
    coro_pool_t coro_pool_;

    order_sink_t order_sink_;
};

}

#endif /* __REPLICATION_BACKFILL_IN_HPP__ */
