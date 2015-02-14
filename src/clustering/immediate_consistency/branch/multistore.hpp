// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_

#include <vector>

#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/interruptor.hpp"
#include "concurrency/one_per_thread.hpp"
#include "containers/scoped.hpp"
#include "protocol_api.hpp"

class binary_blob_t;
class chunk_fun_callback_t;
class metainfo_checker_t;
template <class> class multistore_send_backfill_should_backfill_t;
class mutex_t;
template <class> class region_map_t;
class send_backfill_callback_t;
class store_view_t;
class store_subview_t;
class traversal_progress_combiner_t;
class order_token_t;
class order_checkpoint_t;

template <class> struct new_and_metainfo_checker_t;

class multistore_ptr_t {
public:
    // We don't get ownership of the store_view_t pointers themselves.
    multistore_ptr_t(store_view_t **store_views, int num_store_views,
                     const region_t &region_mask = region_t::universe());

    // Creates a multistore_ptr_t that depends on the lifetime of the
    // inner multistore_ptr_t.
    multistore_ptr_t(multistore_ptr_t *inner,
                     const region_t &region_mask);

    // deletes the store_subview_t objects.
    ~multistore_ptr_t();

    void new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *external_token_out);
    void new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *external_token_out);

    void do_get_metainfo(order_token_t order_token,
                         object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *external_token,
                         signal_t *interruptor,
                         region_map_t<binary_blob_t> *out) THROWS_ONLY(interrupted_exc_t);

    void set_metainfo(const region_map_t<binary_blob_t> &new_metainfo,
                      order_token_t order_token,
                      object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *external_token,
                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    store_view_t *get_store(int i) const;

    int num_stores() const { return store_views_.size(); }

    const region_t &get_region() const { return region_; }

private:
    struct switch_read_token_t;
    region_t get_a_region(int i) const;

    void do_get_a_metainfo(int i,
                           order_token_t order_token,
                           const scoped_array_t<switch_read_token_t> *internal_tokens,
                           signal_t *interruptor,
                           region_map_t<binary_blob_t> *updatee,
                           mutex_t *updatee_mutex) THROWS_NOTHING;

    void do_set_a_metainfo(int i,
                           const region_map_t<binary_blob_t> &new_metainfo,
                           order_token_t order_token,
                           const scoped_array_t<fifo_enforcer_write_token_t> &internal_tokens,
                           signal_t *interruptor);

    void do_initialize(int i, store_view_t **_store_views) THROWS_NOTHING;

    // Used by the constructors.
    void initialize(store_view_t **_store_views) THROWS_NOTHING;

    void switch_read_tokens(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *external_token, signal_t *interruptor, order_token_t *order_token_ref, scoped_array_t<switch_read_token_t> *internal_out);

    void switch_write_tokens(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *external_token, signal_t *interruptor, order_token_t *order_token_ref, scoped_array_t<fifo_enforcer_write_token_t> *internal_out);

    void switch_inner_read_token(int i, fifo_enforcer_read_token_t internal_token, signal_t *interruptor, read_token_t *store_token);

    void switch_inner_write_token(int i, fifo_enforcer_write_token_t internal_token, signal_t *interruptor, write_token_t *store_token);

    // We _own_ these pointers and must delete them at destruction.
    const scoped_array_t<store_view_t *> store_views_;

    const region_t region_;

    // A fifo source/sink pair, exposed publicly in new_read_token and
    // new_write_token.
    one_per_thread_t<fifo_enforcer_source_t> external_source_;
    one_per_thread_t<fifo_enforcer_sink_t> external_sink_;

    one_per_thread_t<order_checkpoint_t> external_checkpoint_;

    // An internal fifo source/sink pair, for cross-thread (and
    // cross-pmap) fifo ordering.  Their tokens are never seen
    // publicly, neither by external users or inner store_ts.
    one_per_thread_t<scoped_array_t<fifo_enforcer_source_t> > internal_sources_;
    scoped_array_t<scoped_ptr_t<fifo_enforcer_sink_t> > internal_sinks_;

    DISABLE_COPYING(multistore_ptr_t);
};




#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_

