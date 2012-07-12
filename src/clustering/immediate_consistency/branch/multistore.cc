#include "clustering/immediate_consistency/branch/multistore.hpp"

#include "errors.hpp"
#include <boost/function.hpp>

#include "btree/parallel_traversal.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "protocol_api.hpp"
#include "rpc/semilattice/joins/vclock.hpp"

template <class protocol_t>
multistore_ptr_t<protocol_t>::multistore_ptr_t(store_view_t<protocol_t> **store_views,
                                               int num_store_views,
                                               const typename protocol_t::region_t &region)
    : store_views_(num_store_views),
      region_(region),
      internal_sources_(num_store_views),
      internal_sinks_(num_store_views) {

    initialize(store_views);
}

template <class protocol_t>
multistore_ptr_t<protocol_t>::multistore_ptr_t(multistore_ptr_t<protocol_t> *inner,
                                               const typename protocol_t::region_t &region)
    : store_views_(inner->num_stores()),
      region_(region),
      internal_sources_(inner->num_stores()),
      internal_sinks_(inner->num_stores()) {
    rassert(region_is_superset(inner->region_, region));

    initialize(inner->store_views_.data());
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::do_initialize(int i, store_view_t<protocol_t> **store_views) THROWS_NOTHING {
    on_thread_t th(store_views[i]->home_thread());

    // We do a region intersection because store_subview_t requires that the region mask be a subset of the store region.
    store_views_[i] = new store_subview_t<protocol_t>(store_views[i],
                                                      region_intersection(region_, store_views[i]->get_region()));

    internal_sinks_[i].init(new fifo_enforcer_sink_t);
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
typename protocol_t::region_t multistore_ptr_t<protocol_t>::get_multistore_joined_region() const {
    return region_;
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::new_read_token(scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *external_token_out) {
    fifo_enforcer_read_token_t token = external_source_.enter_read();
    external_token_out->init(new fifo_enforcer_sink_t::exit_read_t(&external_sink_, token));
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::new_write_token(scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *external_token_out) {
    fifo_enforcer_write_token_t token = external_source_.enter_write();
    external_token_out->init(new fifo_enforcer_sink_t::exit_write_t(&external_sink_, token));
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::do_get_a_metainfo(int i,
                                                     order_token_t order_token,
                                                     const scoped_array_t<fifo_enforcer_read_token_t> &internal_tokens,
                                                     signal_t *interruptor,
                                                     region_map_t<protocol_t, version_range_t> *updatee,
                                                     mutex_t *updatee_mutex) THROWS_NOTHING {
    region_map_t<protocol_t, version_range_t> transformed;

    try {
        {
            const int dest_thread = store_views_[i]->home_thread();
            cross_thread_signal_t ct_interruptor(interruptor, dest_thread);

            on_thread_t th(dest_thread);

            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> store_token;
            switch_inner_read_token(i, internal_tokens[i], &ct_interruptor, &store_token);

            const region_map_t<protocol_t, binary_blob_t>& metainfo
                = store_views_[i]->get_metainfo(order_token, &store_token, &ct_interruptor);
            const region_map_t<protocol_t, binary_blob_t>& masked_metainfo
                = metainfo.mask(get_region(i));

            transformed
                = region_map_transform<protocol_t, binary_blob_t, version_range_t>(masked_metainfo,
                                                                                   &binary_blob_t::get<version_range_t>);
        }

        // updatee->update doesn't block so the mutex is redundant, who cares.
        mutex_t::acq_t acq(updatee_mutex, true);
        updatee->update(transformed);
    } catch (const interrupted_exc_t& exc) {
        // do nothing, we're in pmap.
    }
}

template <class protocol_t>
region_map_t<protocol_t, version_range_t>  multistore_ptr_t<protocol_t>::
get_all_metainfos(order_token_t order_token,
                  scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *external_token,
		  signal_t *interruptor) {

    scoped_array_t<fifo_enforcer_read_token_t> internal_tokens;

    switch_read_tokens(external_token, interruptor, &internal_tokens);

    mutex_t ret_mutex;
    region_map_t<protocol_t, version_range_t> ret(get_multistore_joined_region());

    // TODO: For getting, we possibly want to cache things on the home
    // thread, but wait until we want a multithreaded listener.

    pmap(store_views_.size(), boost::bind(&multistore_ptr_t<protocol_t>::do_get_a_metainfo, this, _1, order_token, boost::ref(internal_tokens), interruptor, &ret, &ret_mutex));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    rassert(ret.get_domain() == region_);

    return ret;
}


template <class protocol_t>
typename protocol_t::region_t multistore_ptr_t<protocol_t>::get_region(int i) const {
    guarantee(0 <= i && i < num_stores());

    return region_intersection(region_, protocol_t::cpu_sharding_subspace(i, num_stores()));
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

        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> store_token;
        switch_inner_write_token(i, internal_tokens[i], &ct_interruptor, &store_token);

        store_views_[i]->set_metainfo(new_metainfo.mask(get_region(i)), order_token, &store_token, &ct_interruptor);

    } catch (const interrupted_exc_t& exc) {
        // do nothing
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::set_all_metainfos(const region_map_t<protocol_t, binary_blob_t> &new_metainfo,
                                                     order_token_t order_token,
                                                     scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *external_token,
                                                     signal_t *interruptor) {
    scoped_array_t<fifo_enforcer_write_token_t> internal_tokens;
    switch_write_tokens(external_token, interruptor, &internal_tokens);

    pmap(num_stores(),
         boost::bind(&multistore_ptr_t<protocol_t>::do_set_a_metainfo, this, _1, boost::ref(new_metainfo), order_token, boost::ref(internal_tokens), interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

template <class protocol_t>
class multistore_send_backfill_should_backfill_t : public home_thread_mixin_t {
public:
    multistore_send_backfill_should_backfill_t(int num_stores, const typename protocol_t::region_t &start_point_region,
                                               const boost::function<bool(const typename protocol_t::store_t::metainfo_t &)> &should_backfill_func)
        : countdown_(num_stores), should_backfill_func_(should_backfill_func), combined_metainfo_(start_point_region) { }

    bool should_backfill(const typename protocol_t::store_t::metainfo_t &metainfo) {
        on_thread_t th(home_thread());

        combined_metainfo_.update(metainfo);

        --countdown_;
        rassert(countdown_ >= 0, "countdown_ is %d\n", countdown_);

        if (countdown_ == 0) {
            bool tmp = should_backfill_func_(combined_metainfo_);
            result_promise_.pulse(tmp);
        }

        bool res = result_promise_.wait();

        return res;
    }

    bool get_result() {
        guarantee(result_promise_.is_pulsed());
        return result_promise_.wait();
    }

private:
    int countdown_;
    const boost::function<bool(const typename protocol_t::store_t::metainfo_t &)> &should_backfill_func_;
    promise_t<bool> result_promise_;
    typename protocol_t::store_t::metainfo_t combined_metainfo_;

    DISABLE_COPYING(multistore_send_backfill_should_backfill_t);
};

template <class protocol_t>
void regionwrap_chunkfun(const boost::function<void(typename protocol_t::backfill_chunk_t)> &wrappee, int target_thread, const typename protocol_t::region_t& region, typename protocol_t::backfill_chunk_t chunk) {
    // TODO: Is chunkfun supposed to block like this?
    on_thread_t th(target_thread);

    // TODO: This is a borderline hack for memcached delete_range_t chunks.
    wrappee(chunk.shard(region));
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::single_shard_backfill(int i,
                                                         multistore_send_backfill_should_backfill_t<protocol_t> *helper,
                                                         const region_map_t<protocol_t, state_timestamp_t> &start_point,
                                                         const boost::function<void(typename protocol_t::backfill_chunk_t)> &chunk_fun,
                                                         UNUSED typename protocol_t::backfill_progress_t *progress,
                                                         const scoped_array_t<fifo_enforcer_read_token_t> &internal_tokens,
                                                         signal_t *interruptor) THROWS_NOTHING {
    store_view_t<protocol_t> *store = store_views_[i];

    const int chunk_fun_target_hread = get_thread_id();
    const int dest_thread = store->home_thread();

    cross_thread_signal_t ct_interruptor(interruptor, dest_thread);

    on_thread_t th(dest_thread);

    // TODO: Fix the passing of progress.
    typename protocol_t::backfill_progress_t tmp_progress;

    try {

        scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> store_token;
        switch_inner_read_token(i, internal_tokens[i], &ct_interruptor, &store_token);

        store->send_backfill(start_point.mask(get_region(i)),
                             boost::bind(&multistore_send_backfill_should_backfill_t<protocol_t>::should_backfill, helper, _1),
                             boost::bind(regionwrap_chunkfun<protocol_t>, chunk_fun, chunk_fun_target_hread, get_region(i), _1),
                             &tmp_progress,
                             &store_token,
                             &ct_interruptor);
    } catch (const interrupted_exc_t& exc) {
        // do nothing
    }
}

/* This has to be compatible with the conditions given by
   send_backfill in protocol_api.hpp.  Here is a copy that might be
   out-of-date.

   Expresses the changes that have happened since `start_point` as a
   series of `backfill_chunk_t` objects.
   [Precondition] start_point.get_domain() <= view->get_region()
   [Side-effect] `should_backfill` must be called exactly once
   [Return value] Value equal to the value returned by should_backfill
   [May block]
*/
template <class protocol_t>
bool multistore_ptr_t<protocol_t>::send_multistore_backfill(const region_map_t<protocol_t, state_timestamp_t> &start_point,
                                                            const boost::function<bool(const typename protocol_t::store_t::metainfo_t &)> &should_backfill,
                                                            const boost::function<void(typename protocol_t::backfill_chunk_t)> &chunk_fun,
                                                            typename protocol_t::backfill_progress_t *progress,
                                                            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *external_token,
                                                            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    guarantee(region_is_superset(get_multistore_joined_region(), start_point.get_domain()));

    scoped_array_t<fifo_enforcer_read_token_t> internal_tokens;
    switch_read_tokens(external_token, interruptor, &internal_tokens);

    multistore_send_backfill_should_backfill_t<protocol_t> helper(num_stores(), start_point.get_domain(), should_backfill);

    pmap(num_stores(), boost::bind(&multistore_ptr_t<protocol_t>::single_shard_backfill,
                                   this,
                                   _1,
                                   &helper,
                                   boost::ref(start_point),
                                   boost::ref(chunk_fun),
                                   progress,
                                   boost::ref(internal_tokens),
                                   interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    return helper.get_result();
}

// TODO: Add order_token_t to this.
template <class protocol_t>
void multistore_ptr_t<protocol_t>::single_shard_receive_backfill(int i, const typename protocol_t::backfill_chunk_t &chunk,
                                                                 const scoped_array_t<fifo_enforcer_write_token_t> &internal_tokens,
                                                                 signal_t *interruptor) THROWS_NOTHING {

    typename protocol_t::region_t ith_intersection = region_intersection(get_region(i), chunk.get_region());

    store_view_t<protocol_t> *store = store_views_[i];
    const int dest_thread = store->home_thread();


    cross_thread_signal_t ct_interruptor(interruptor, dest_thread);
    on_thread_t th(dest_thread);

    try {
        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> store_token;
        switch_inner_write_token(i, internal_tokens[i], &ct_interruptor, &store_token);

        if (region_is_empty(ith_intersection)) {
            // TODO: We shouldn't have to switch threads to find the
            // empty intersection and destroy the store_token.  Don't
            // get an internal token in the first place.
            return;
        }

        store->receive_backfill(chunk.shard(ith_intersection),
                                &store_token,
                                &ct_interruptor);
    } catch (const interrupted_exc_t& exc) {
        // do nothing
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::receive_backfill(const typename protocol_t::backfill_chunk_t &chunk,
                                                    scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *external_token,
                                                    signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    guarantee(region_is_superset(get_multistore_joined_region(), chunk.get_region()));

    scoped_array_t<fifo_enforcer_write_token_t> internal_tokens;
    switch_write_tokens(external_token, interruptor, &internal_tokens);

    pmap(num_stores(), boost::bind(&multistore_ptr_t<protocol_t>::single_shard_receive_backfill,
                                   this,
                                   _1,
                                   boost::ref(chunk),
                                   boost::ref(internal_tokens),
                                   interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}




template <class protocol_t>
void multistore_ptr_t<protocol_t>::single_shard_read(int i,
                                                     DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
                                                     const typename protocol_t::read_t &read,
                                                     order_token_t order_token,
                                                     const scoped_array_t<fifo_enforcer_read_token_t> &internal_tokens,
                                                     std::vector<typename protocol_t::read_response_t> *responses,
                                                     signal_t *interruptor) THROWS_NOTHING {
    const typename protocol_t::region_t ith_region = get_region(i);
    typename protocol_t::region_t ith_intersection = region_intersection(ith_region, read.get_region());

    const int dest_thread = store_views_[i]->home_thread();

    cross_thread_signal_t ct_interruptor(interruptor, dest_thread);

    try {
        typename protocol_t::read_response_t response;

        {
            on_thread_t th(dest_thread);

            scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> store_token;
            switch_inner_read_token(i, internal_tokens[i], &ct_interruptor, &store_token);

            if (region_is_empty(ith_intersection)) {
                // TODO: This is ridiculous.  We don't have to go to
                // this thread to find out that the region is empty
                // and kill the store token if we don't create the
                // internal token in the first place.
                return;
            }

            response = store_views_[i]->read(DEBUG_ONLY(metainfo_checker.mask(ith_region), )
                                             read.shard(ith_intersection),
                                             order_token,
                                             &store_token,
                                             &ct_interruptor);
        }

        responses->push_back(response);
    } catch (const interrupted_exc_t& exc) {
        // do nothing
    }
}

template <class protocol_t>
typename protocol_t::read_response_t
multistore_ptr_t<protocol_t>::read(DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
                                   const typename protocol_t::read_t &read,
                                   order_token_t order_token,
                                   scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *external_token,
                                   signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    scoped_array_t<fifo_enforcer_read_token_t> internal_tokens;
    switch_read_tokens(external_token, interruptor, &internal_tokens);

    std::vector<typename protocol_t::read_response_t> responses;
    pmap(num_stores(), boost::bind(&multistore_ptr_t<protocol_t>::single_shard_read,
                                   this, _1, DEBUG_ONLY(boost::ref(metainfo_checker), )
                                   boost::ref(read),
                                   order_token,
                                   boost::ref(internal_tokens),
                                   &responses,
                                   interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    typename protocol_t::temporary_cache_t fake_cache;
    return read.multistore_unshard(responses, &fake_cache);
}

// Because boost::bind only takes 10 arguments.
template <class protocol_t>
struct new_and_metainfo_checker_t {
    const typename protocol_t::store_t::metainfo_t &new_metainfo;
#ifndef NDEBUG
    const metainfo_checker_t<protocol_t> &metainfo_checker;
    new_and_metainfo_checker_t(const metainfo_checker_t<protocol_t> &_metainfo_checker,
                               const typename protocol_t::store_t::metainfo_t &_new_metainfo)
        : new_metainfo(_new_metainfo), metainfo_checker(_metainfo_checker) { }
#else
    explicit new_and_metainfo_checker_t(const typename protocol_t::store_t::metainfo_t &_new_metainfo)
        : new_metainfo(_new_metainfo) { }
#endif
};

template <class protocol_t>
void multistore_ptr_t<protocol_t>::single_shard_write(int i,
                                                      const new_and_metainfo_checker_t<protocol_t>& metainfo,
                                                      const typename protocol_t::write_t &write,
                                                      transition_timestamp_t timestamp,
                                                      order_token_t order_token,
                                                      const scoped_array_t<fifo_enforcer_write_token_t> &internal_tokens,
                                                      std::vector<typename protocol_t::write_response_t> *responses,
                                                      signal_t *interruptor) THROWS_NOTHING {
    const typename protocol_t::region_t &ith_region = get_region(i);
    typename protocol_t::region_t ith_intersection = region_intersection(ith_region, write.get_region());

    const int dest_thread = store_views_[i]->home_thread();
    cross_thread_signal_t ct_interruptor(interruptor, dest_thread);

    on_thread_t th(dest_thread);

    // TODO: Have an assertion about the new_metainfo region?

    try {
        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> store_token;
        switch_inner_write_token(i, internal_tokens[i], &ct_interruptor, &store_token);

        if (region_is_empty(ith_intersection)) {
            // TODO: Having to go to this thread to do nothing on empty regions is ridiculous.
            return;
        }

        responses->push_back(store_views_[i]->write(DEBUG_ONLY(metainfo.metainfo_checker.mask(ith_region), )
                                                    metainfo.new_metainfo.mask(ith_region),
                                                    write.shard(ith_intersection),
                                                    timestamp,
                                                    order_token,
                                                    &store_token,
                                                    &ct_interruptor));
    } catch (const interrupted_exc_t& exc) {
        // do nothing
    }
}

template <class protocol_t>
typename protocol_t::write_response_t
multistore_ptr_t<protocol_t>::write(DEBUG_ONLY(const metainfo_checker_t<protocol_t>& metainfo_checker, )
                                    const typename protocol_t::store_t::metainfo_t& new_metainfo,
                                    const typename protocol_t::write_t &write,
                                    transition_timestamp_t timestamp,
                                    order_token_t order_token,
                                    scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *external_token,
                                    signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    scoped_array_t<fifo_enforcer_write_token_t> internal_tokens;
    switch_write_tokens(external_token, interruptor, &internal_tokens);

    std::vector<typename protocol_t::write_response_t> responses;
    new_and_metainfo_checker_t<protocol_t> metainfo(DEBUG_ONLY(metainfo_checker, ) new_metainfo);
    pmap(num_stores(), boost::bind(&multistore_ptr_t<protocol_t>::single_shard_write,
                                   this, _1, boost::ref(metainfo),
                                   boost::ref(write),
                                   timestamp,
                                   order_token,
                                   boost::ref(internal_tokens),
                                   &responses,
                                   interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    typename protocol_t::temporary_cache_t fake_cache;
    return write.multistore_unshard(responses, &fake_cache);
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::single_shard_reset_all_data(int i,
                                                               const typename protocol_t::region_t &subregion,
                                                               const typename protocol_t::store_t::metainfo_t &new_metainfo,
                                                               const scoped_array_t<fifo_enforcer_write_token_t> &internal_tokens,
                                                               signal_t *interruptor) THROWS_NOTHING {
    const int dest_thread = store_views_[i]->home_thread();

    cross_thread_signal_t ct_interruptor(interruptor, dest_thread);

    on_thread_t th(dest_thread);

    try {
        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> store_token;
        switch_inner_write_token(i, internal_tokens[i], &ct_interruptor, &store_token);

        if (!region_overlaps(get_region(i), subregion)) {
            // TODO: We shouldn't have to go to this thread if we do nothing.
            return;
        }


        // TODO: order token?
        store_views_[i]->reset_data(region_intersection(subregion, get_region(i)),
                                    new_metainfo.mask(get_region(i)),
                                    &store_token,
                                    &ct_interruptor);
    } catch (const interrupted_exc_t& exc) {
        // do nothing
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::reset_all_data(const typename protocol_t::region_t &subregion,
                                                  const typename protocol_t::store_t::metainfo_t &new_metainfo,
                                                  scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *external_token,
                                                  signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    scoped_array_t<fifo_enforcer_write_token_t> internal_tokens;
    switch_write_tokens(external_token, interruptor, &internal_tokens);

    pmap(num_stores(), boost::bind(&multistore_ptr_t<protocol_t>::single_shard_reset_all_data,
                                   this, _1,
                                   boost::ref(subregion),
                                   boost::ref(new_metainfo),
                                   boost::ref(internal_tokens),
                                   interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::switch_read_tokens(scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *external_token, signal_t *interruptor, scoped_array_t<fifo_enforcer_read_token_t> *internal_out) {
    scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> local(external_token->release());

    wait_interruptible(local.get(), interruptor);

    const int n = num_stores();
    internal_out->init(n);
    for (int i = 0; i < n; ++i) {
        (*internal_out)[i] = internal_sources_[i].enter_read();
    }
}


template <class protocol_t>
void multistore_ptr_t<protocol_t>::switch_write_tokens(scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *external_token, signal_t *interruptor, scoped_array_t<fifo_enforcer_write_token_t> *internal_out) {
    scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> local(external_token->release());

    wait_interruptible(local.get(), interruptor);

    const int n = num_stores();
    internal_out->init(n);
    for (int i = 0; i < n; ++i) {
        (*internal_out)[i] = internal_sources_[i].enter_write();
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::switch_inner_read_token(int i, fifo_enforcer_read_token_t internal_token, signal_t *interruptor, scoped_ptr_t<fifo_enforcer_sink_t::exit_read_t> *store_token) {
    fifo_enforcer_sink_t *internal_sink = internal_sinks_[i].get();
    internal_sink->assert_thread();

    fifo_enforcer_sink_t::exit_read_t internal_exit(internal_sink, internal_token);
    wait_interruptible(&internal_exit, interruptor);

    store_views_[i]->new_read_token(store_token);
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::switch_inner_write_token(int i, fifo_enforcer_write_token_t internal_token, signal_t *interruptor, scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> *store_token) {
    fifo_enforcer_sink_t *internal_sink = internal_sinks_[i].get();
    internal_sink->assert_thread();

    fifo_enforcer_sink_t::exit_write_t internal_exit(internal_sink, internal_token);
    wait_interruptible(&internal_exit, interruptor);

    store_views_[i]->new_write_token(store_token);
}






#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template class multistore_ptr_t<mock::dummy_protocol_t>;
template class multistore_ptr_t<memcached_protocol_t>;
