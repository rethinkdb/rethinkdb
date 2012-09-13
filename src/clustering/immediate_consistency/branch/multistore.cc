#include "clustering/immediate_consistency/branch/multistore.hpp"

#include "errors.hpp"
#include <boost/function.hpp>

#include "btree/parallel_traversal.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "protocol_api.hpp"
#include "rpc/semilattice/joins/vclock.hpp"

template <class protocol_t>
struct multistore_ptr_t<protocol_t>::switch_read_token_t {
    bool do_read;
    int shard;
    typename protocol_t::region_t region;
    typename protocol_t::region_t intersection;
    fifo_enforcer_read_token_t token;
};

template <class protocol_t>
store_view_t<protocol_t> *multistore_ptr_t<protocol_t>::get_store(int i) const {
    guarantee(0 <= i && i < num_stores());
    return store_views_[i];
}

template <class protocol_t>
region_map_t<protocol_t, version_range_t> to_version_range_map(const region_map_t<protocol_t, binary_blob_t> &blob_map) {
    return region_map_transform<protocol_t, binary_blob_t, version_range_t>(blob_map,
                                                                            &binary_blob_t::get<version_range_t>);
}

template <class protocol_t>
multistore_ptr_t<protocol_t>::multistore_ptr_t(store_view_t<protocol_t> **store_views, int num_store_views,
                                               typename protocol_t::context_t *_ctx,
                                               const typename protocol_t::region_t &region)
    : store_views_(num_store_views),
      // TODO: region_ is redundant with superclass's region.
      region_(region),
      external_checkpoint_("multistore_ptr_t"),
      internal_sources_(num_store_views),
      internal_sinks_(num_store_views),
      ctx(_ctx) {

    initialize(store_views);
}

template <class protocol_t>
multistore_ptr_t<protocol_t>::multistore_ptr_t(multistore_ptr_t<protocol_t> *inner,
                                               typename protocol_t::context_t *_ctx,
                                               const typename protocol_t::region_t &region)
    : store_views_(inner->num_stores()),
      region_(region),
      external_checkpoint_("multistore_ptr_t"),
      internal_sources_(inner->num_stores()),
      internal_sinks_(inner->num_stores()),
      ctx(_ctx) {

    rassert(region_is_superset(inner->region_, region));

    initialize(inner->store_views_.data());
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::do_initialize(int i, store_view_t<protocol_t> **store_views) THROWS_NOTHING {
    on_thread_t th(store_views[i]->home_thread());

    // We do a region intersection because store_subview_t requires that the region mask be a subset of the store region.
    store_views_[i] = new store_subview_t<protocol_t>(store_views[i],
                                                      region_intersection(protocol_t::cpu_sharding_subspace(i, num_stores()),
 region_intersection(region_, store_views[i]->get_region())));

    // We have an internal sink for each thread of internal_sources.
    // However really one of them goes unused, because
    // one_per_thread_t uses get_num_threads instead of
    // get_num_db_threads.
    internal_sinks_[i].init(new fifo_enforcer_sink_t);

    // TODO: Should we use get_num_threads() or get_num_db_threads()?  What about in one_per_thread_t?
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::initialize(store_view_t<protocol_t> **store_views) THROWS_NOTHING {
    pmap(store_views_.size(), boost::bind(&multistore_ptr_t<protocol_t>::do_initialize, this, _1, store_views));
}

template <class protocol_t>
void do_destroy(int i, store_view_t<protocol_t> **store_views) {
    guarantee(store_views[i] != NULL);

    on_thread_t th(store_views[i]->home_thread());

    delete store_views[i];
    store_views[i] = NULL;
}

template <class protocol_t>
multistore_ptr_t<protocol_t>::~multistore_ptr_t() {
    pmap(store_views_.size(), boost::bind(do_destroy<protocol_t>, _1, store_views_.data()));
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *external_token_out) {
    fifo_enforcer_read_token_t token = external_source_.get()->enter_read();
    external_token_out->create(external_sink_.get(), token);
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *external_token_out) {
    fifo_enforcer_write_token_t token = external_source_.get()->enter_write();
    external_token_out->create(external_sink_.get(), token);
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::do_get_a_metainfo(int i,
                                                     order_token_t order_token,
                                                     const scoped_array_t<switch_read_token_t> *internal_tokens,
                                                     signal_t *interruptor,
                                                     region_map_t<protocol_t, binary_blob_t> *updatee,
                                                     mutex_t *updatee_mutex) THROWS_NOTHING {
    region_map_t<protocol_t, binary_blob_t> metainfo;

    try {
        {
            const int dest_thread = store_views_[i]->home_thread();
            cross_thread_signal_t ct_interruptor(interruptor, dest_thread);

            on_thread_t th(dest_thread);

            object_buffer_t<fifo_enforcer_sink_t::exit_read_t> store_token;
            switch_inner_read_token(i, internal_tokens->data()[i].token, &ct_interruptor, &store_token);

            store_views_[i]->do_get_metainfo(order_token, &store_token, &ct_interruptor, &metainfo);
        }

        // updatee->update doesn't block so the mutex is redundant, who cares.
        mutex_t::acq_t acq(updatee_mutex, true);
        updatee->update(metainfo.mask(get_a_region(i)));
    } catch (const interrupted_exc_t& exc) {
        // do nothing, we're in pmap.
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::do_get_metainfo(order_token_t order_token,
                                                   object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *external_token,
                                                   signal_t *interruptor,
                                                   region_map_t<protocol_t, binary_blob_t> *out)
    THROWS_ONLY(interrupted_exc_t) {

    // RSI: this is kind of awkward
    int count = num_stores();
    scoped_array_t<switch_read_token_t> internal_tokens(count);

    for (int i = 0; i < count; ++i) {
        internal_tokens[i].do_read = true;
    }

    switch_read_tokens(external_token, interruptor, &order_token, &internal_tokens);

    mutex_t out_mutex;
    typename protocol_t::region_t region = this->get_region();

    {
        binary_blob_t blob((version_range_t(version_t::zero())));
        region_map_t<protocol_t, binary_blob_t> ret(region, blob);
        *out = ret;
    }

    // TODO: For getting, we possibly want to cache things on the home
    // thread, but wait until we want a multithreaded listener.

    pmap(count, boost::bind(&multistore_ptr_t<protocol_t>::do_get_a_metainfo,
                            this, _1, order_token, &internal_tokens, interruptor, out, &out_mutex));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    rassert(out->get_domain() == region_);
}


template <class protocol_t>
typename protocol_t::region_t multistore_ptr_t<protocol_t>::get_a_region(int i) const {
    guarantee(0 <= i && i < num_stores());

    return store_views_[i]->get_region();
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::do_set_a_metainfo(int i,
                                                     const region_map_t<protocol_t, binary_blob_t> &new_metainfo,
                                                     order_token_t order_token,
                                                     const scoped_array_t<fifo_enforcer_write_token_t> &internal_tokens,
                                                     signal_t *interruptor) {

    try {

        const int dest_thread = store_views_[i]->home_thread();
        cross_thread_signal_t ct_interruptor(interruptor, dest_thread);

        on_thread_t th(dest_thread);

        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> store_token;
        switch_inner_write_token(i, internal_tokens[i], &ct_interruptor, &store_token);

        store_views_[i]->set_metainfo(new_metainfo.mask(get_a_region(i)), order_token, &store_token, &ct_interruptor);

    } catch (const interrupted_exc_t& exc) {
        // do nothing
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::set_metainfo(const region_map_t<protocol_t, binary_blob_t> &new_metainfo,
                                                order_token_t order_token,
                                                object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *external_token,
                                                signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    scoped_array_t<fifo_enforcer_write_token_t> internal_tokens;
    switch_write_tokens(external_token, interruptor, &order_token, &internal_tokens);

    pmap(num_stores(),
         boost::bind(&multistore_ptr_t<protocol_t>::do_set_a_metainfo, this, _1, boost::ref(new_metainfo), order_token, boost::ref(internal_tokens), interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::switch_read_tokens(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *external_token, signal_t *interruptor, order_token_t *order_token_ref, scoped_array_t<switch_read_token_t> *internal_out) {
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(external_token);

    wait_interruptible(external_token->get(), interruptor);

    *order_token_ref = external_checkpoint_.get()->check_through(*order_token_ref);

    const int n = num_stores();
    for (int i = 0; i < n; ++i) {
        if (internal_out->data()[i].do_read) {
            internal_out->data()[i].token = (*internal_sources_.get())[i].enter_read();
        }
    }
}


template <class protocol_t>
void multistore_ptr_t<protocol_t>::switch_write_tokens(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *external_token, signal_t *interruptor, order_token_t *order_token_ref, scoped_array_t<fifo_enforcer_write_token_t> *internal_out) {
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(external_token);

    wait_interruptible(external_token->get(), interruptor);

    *order_token_ref = external_checkpoint_.get()->check_through(*order_token_ref);

    const int n = num_stores();
    internal_out->init(n);
    for (int i = 0; i < n; ++i) {
        (*internal_out)[i] = (*internal_sources_.get())[i].enter_write();
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::switch_inner_read_token(int i, fifo_enforcer_read_token_t internal_token, signal_t *interruptor, object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *store_token) {
    fifo_enforcer_sink_t *internal_sink = internal_sinks_[i].get();
    internal_sink->assert_thread();

    fifo_enforcer_sink_t::exit_read_t internal_exit(internal_sink, internal_token);
    wait_interruptible(&internal_exit, interruptor);

    store_views_[i]->new_read_token(store_token);
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::switch_inner_write_token(int i, fifo_enforcer_write_token_t internal_token, signal_t *interruptor, object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *store_token) {
    fifo_enforcer_sink_t *internal_sink = internal_sinks_[i].get();
    internal_sink->assert_thread();

    fifo_enforcer_sink_t::exit_write_t internal_exit(internal_sink, internal_token);
    wait_interruptible(&internal_exit, interruptor);

    store_views_[i]->new_write_token(store_token);
}

#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"
#include "rdb_protocol/protocol.hpp"

template class multistore_ptr_t<mock::dummy_protocol_t>;
template class multistore_ptr_t<memcached_protocol_t>;
template class multistore_ptr_t<rdb_protocol_t>;

template region_map_t<mock::dummy_protocol_t, version_range_t> to_version_range_map<mock::dummy_protocol_t>(const region_map_t<mock::dummy_protocol_t, binary_blob_t> &blob_map);
template region_map_t<memcached_protocol_t, version_range_t> to_version_range_map<memcached_protocol_t>(const region_map_t<memcached_protocol_t, binary_blob_t> &blob_map);
template region_map_t<rdb_protocol_t, version_range_t> to_version_range_map<rdb_protocol_t>(const region_map_t<rdb_protocol_t, binary_blob_t> &blob_map);
