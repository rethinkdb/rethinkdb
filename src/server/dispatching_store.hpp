#ifndef __SERVER_DISPATCHING_STORE_HPP__
#define __SERVER_DISPATCHING_STORE_HPP__

#include "store.hpp"
#include "concurrency/fifo_checker.hpp"

class btree_slice_t;


class dispatching_store_t : public set_store_t, public home_thread_mixin_t {
public:
    dispatching_store_t(btree_slice_t *substore);
    void set_dispatcher(boost::function<mutation_t(const mutation_t &, castime_t, order_token_t)> disp);

    mutation_result_t change(const mutation_t& m, castime_t castime, order_token_t token);
    get_result_t get(const store_key_t &key, order_token_t token);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token);
    void delete_all_keys_for_backfill(order_token_t token);
    void set_replication_clock(repli_timestamp_t t, order_token_t token);

private:
    order_source_t order_source_for_dispatchee;
    boost::function<mutation_t(const mutation_t&, castime_t, order_token_t)> dispatcher;
    btree_slice_t *substore;


    order_sink_t sink;
    order_source_t pre_dispatch_source;
    plain_sink_t post_dispatch_sink;

public:
    // for slice_manager_t, ugh.
    order_source_t substore_order_source;
};

#endif /* __SERVER_DISPATCHING_STORE_HPP__ */
