#ifndef __BTREE_SLICE_HPP__
#define __BTREE_SLICE_HPP__

#include "store.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "serializer/translator.hpp"

class mutation_dispatcher_t;

class initialize_superblock_fsm_t;
struct btree_replicant_t;
class backfill_callback_t;

class mutation_dispatcher_t : public intrusive_list_node_t<mutation_dispatcher_t> {
public:
    mutation_dispatcher_t() { }
    // Dispatches a change, and returns a modified mutation_t.
    virtual mutation_t dispatch_change(const mutation_t& m, castime_t castime) = 0;
    virtual ~mutation_dispatcher_t() { }
private:
    DISABLE_COPYING(mutation_dispatcher_t);
};


/* btree_slice_t is a thin wrapper around cache_t that handles initializing the buffer
cache for the purpose of storing a btree. There are many btree_slice_ts per
btree_key_value_store_t. */

class btree_slice_t :
    public get_store_t,
    public set_store_t,
    public home_thread_mixin_t
{
public:
    // Blocks
    static void create(translator_serializer_t *serializer,
                       mirrored_cache_static_config_t *static_config);

    // Blocks
    btree_slice_t(translator_serializer_t *serializer,
                  mirrored_cache_config_t *dynamic_config);

    // Blocks
    ~btree_slice_t();

    /* get_store_t interface. */

    get_result_t get(const store_key_t &key);
    rget_result_t rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key);

    /* set_store_t interface */

    mutation_result_t change(const mutation_t &m, castime_t castime);

    void time_barrier(repli_timestamp lower_bound_on_future_timestamps);

    /* backfill interface, so to speak */
    void backfill(repli_timestamp since_when, backfill_callback_t *callback);

    /* real-time monitoring interface (used for replication) */
    void add_dispatcher(mutation_dispatcher_t *mdisp);
    void remove_dispatcher(mutation_dispatcher_t *mdisp);

    cache_t *cache() { return &cache_; }

private:
    cache_t cache_;
    intrusive_list_t<mutation_dispatcher_t> dispatchers_;

    DISABLE_COPYING(btree_slice_t);
};

#endif /* __BTREE_SLICE_HPP__ */
