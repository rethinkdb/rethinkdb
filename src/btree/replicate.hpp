#ifndef __BTREE_WALK_HPP__
#define __BTREE_WALK_HPP__

#include "store.hpp"

struct btree_key_value_store_t;
struct btree_slice_t;
struct slice_walker_t;

/* btree_replicant_t manages the registration of a store_t::replicant_t with a 
btree_key_value_store_t. When you call btree_key_value_store_t::replicate(), a
btree_replicant_t is created. When you call btree_key_value_store_t::stop_replicating(),
it is destroyed. */

struct btree_replicant_t :
    public home_cpu_mixin_t
{
    store_t::replicant_t *callback;
    btree_key_value_store_t *store;
    repli_timestamp cutoff_recency;

    btree_replicant_t(store_t::replicant_t *, btree_key_value_store_t *, repli_timestamp);
    void stop();

private:

    friend class slice_walker_t;
    void slice_walker_done();

    bool install(btree_slice_t *);
    bool uninstall(btree_slice_t *);
    bool have_uninstalled();
    void done();

    int active_slice_walkers, active_uninstallations;
    bool stopping;
};

#endif /* __BTREE_WALK_HPP__ */
