#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_

#include <vector>

#include "errors.hpp"
#include <boost/function.hpp>

#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/one_per_thread.hpp"
#include "containers/scoped.hpp"

namespace boost { template <class> class function; }
class binary_blob_t;
template <class> class chunk_fun_callback_t;
template <class> class metainfo_checker_t;
template <class> class multistore_send_backfill_should_backfill_t;
class mutex_t;
template <class, class> class region_map_t;
template <class> class send_backfill_callback_t;
template <class> class store_view_t;
template <class> class store_subview_t;
class traversal_progress_combiner_t;
class order_token_t;
class order_checkpoint_t;
class version_range_t;

template <class> struct new_and_metainfo_checker_t;

template <class protocol_t>
region_map_t<protocol_t, version_range_t> to_version_range_map(const region_map_t<protocol_t, binary_blob_t> &blob_map);

template <class protocol_t>
class multistore_ptr_t {
public:
    // We don't get ownership of the store_view_t pointers themselves.
    multistore_ptr_t(store_view_t<protocol_t> **store_views, int num_store_views,
                     typename protocol_t::context_t *ctx,
                     const typename protocol_t::region_t &region_mask = protocol_t::region_t::universe());

    // Creates a multistore_ptr_t that depends on the lifetime of the
    // inner multistore_ptr_t.
    multistore_ptr_t(multistore_ptr_t<protocol_t> *inner,
                     typename protocol_t::context_t *ctx,
                     const typename protocol_t::region_t &region_mask);

    // deletes the store_subview_t objects.
    ~multistore_ptr_t();

    void new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *external_token_out);
    void new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *external_token_out);

    void do_get_metainfo(order_token_t order_token,
                         object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *external_token,
                         signal_t *interruptor,
                         region_map_t<protocol_t, binary_blob_t> *out) THROWS_ONLY(interrupted_exc_t);

    void set_metainfo(const region_map_t<protocol_t, binary_blob_t> &new_metainfo,
                      order_token_t order_token,
                      object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *external_token,
                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    store_view_t<protocol_t> *get_store(int i) const;

    int num_stores() const { return store_views_.size(); }

    const typename protocol_t::region_t &get_region() const { return region_; }

private:
    struct switch_read_token_t;
    typename protocol_t::region_t get_a_region(int i) const;

    // Used by send_multistore_backfill.
    void single_shard_backfill(int i,
                               multistore_send_backfill_should_backfill_t<protocol_t> *helper,
                               const region_map_t<protocol_t, state_timestamp_t> &start_point,
                               chunk_fun_callback_t<protocol_t> *chunk_fun_cb,
                               traversal_progress_combiner_t *progress,
                               const scoped_array_t<switch_read_token_t> *internal_tokens,
                               signal_t *interruptor) THROWS_NOTHING;

    void single_shard_receive_backfill(int i, const typename protocol_t::backfill_chunk_t &chunk,
                                       const scoped_array_t<fifo_enforcer_write_token_t> &internal_tokens,
                                       signal_t *interruptor) THROWS_NOTHING;

    void single_shard_read(DEBUG_ONLY(const metainfo_checker_t<protocol_t> &metainfo_checker,)
                           const typename protocol_t::read_t *read,
                           order_token_t order_token,
                           const typename multistore_ptr_t<protocol_t>::switch_read_token_t *read_info,
                           typename protocol_t::read_response_t *response,
                           size_t *reads_left,
                           cond_t *done,
                           signal_t *interruptor) THROWS_NOTHING;

    void single_shard_write(int i,
                            const new_and_metainfo_checker_t<protocol_t> &metainfo,
                            const typename protocol_t::write_t &write,
                            transition_timestamp_t timestamp,
                            order_token_t order_token,
                            const scoped_array_t<fifo_enforcer_write_token_t> &internal_tokens,
                            std::vector<typename protocol_t::write_response_t> *responses,
                            signal_t *interruptor) THROWS_NOTHING;

    void single_shard_reset_data(int i,
                                 const typename protocol_t::region_t &subregion,
                                 const typename protocol_t::store_t::metainfo_t &new_metainfo,
                                 const scoped_array_t<fifo_enforcer_write_token_t> &internal_tokens,
                                 signal_t *interruptor) THROWS_NOTHING;

    void do_get_a_metainfo(int i,
                           order_token_t order_token,
                           const scoped_array_t<switch_read_token_t> *internal_tokens,
                           signal_t *interruptor,
                           region_map_t<protocol_t, binary_blob_t> *updatee,
                           mutex_t *updatee_mutex) THROWS_NOTHING;

    void do_set_a_metainfo(int i,
                           const region_map_t<protocol_t, binary_blob_t> &new_metainfo,
                           order_token_t order_token,
                           const scoped_array_t<fifo_enforcer_write_token_t> &internal_tokens,
                           signal_t *interruptor);

    void do_initialize(int i, store_view_t<protocol_t> **_store_views) THROWS_NOTHING;

    // Used by the constructors.
    void initialize(store_view_t<protocol_t> **_store_views) THROWS_NOTHING;

    void switch_read_tokens(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *external_token, signal_t *interruptor, order_token_t *order_token_ref, scoped_array_t<switch_read_token_t> *internal_out);

    void switch_write_tokens(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *external_token, signal_t *interruptor, order_token_t *order_token_ref, scoped_array_t<fifo_enforcer_write_token_t> *internal_out);

    void switch_inner_read_token(int i, fifo_enforcer_read_token_t internal_token, signal_t *interruptor, object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *store_token);

    void switch_inner_write_token(int i, fifo_enforcer_write_token_t internal_token, signal_t *interruptor, object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *store_token);

    // We _own_ these pointers and must delete them at destruction.
    const scoped_array_t<store_view_t<protocol_t> *> store_views_;

    const typename protocol_t::region_t region_;

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

    typename protocol_t::context_t *ctx;

    DISABLE_COPYING(multistore_ptr_t);
};




#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_

