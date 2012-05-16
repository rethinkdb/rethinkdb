#include "clustering/immediate_consistency/branch/multistore.hpp"

#include "errors.hpp"
#include <boost/function.hpp>

#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "protocol_api.hpp"
#include "rpc/semilattice/joins/vclock.hpp"

template <class protocol_t>
typename protocol_t::region_t compute_unmasked_multistore_joined_region(store_view_t<protocol_t> *const *store_views, int num_store_views) {
    std::vector<typename protocol_t::region_t> regions;
    for (int i = 0; i < num_store_views; ++i) {
        regions.push_back(store_views[i]->get_region());
    }

    typename protocol_t::region_t reg;
    region_join_result_t res = region_join(regions, &reg);
    // TODO: This is what's supposed to happen, but is this really guaranteed?  (I think it is not.)
    guarantee(res == REGION_JOIN_OK);

    return reg;
}

template <class protocol_t>
multistore_ptr_t<protocol_t>::multistore_ptr_t(store_view_t<protocol_t> **_store_views, int num_store_views, const typename protocol_t::region_t &_region_mask)
    : store_views(num_store_views, NULL)
      DEBUG_ONLY(, region_mask(_region_mask)) {

    rassert(region_is_superset(compute_unmasked_multistore_joined_region(_store_views, num_store_views), _region_mask));

    initialize(_store_views, _region_mask);
}

template <class protocol_t>
multistore_ptr_t<protocol_t>::multistore_ptr_t(multistore_ptr_t<protocol_t> *inner, const typename protocol_t::region_t &_region_mask)
    : store_views(inner->num_stores(), NULL)
      DEBUG_ONLY(, region_mask(_region_mask)) {
    rassert(region_is_superset(inner->region_mask, _region_mask));

    initialize(inner->store_views.data(), _region_mask);
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::initialize(store_view_t<protocol_t> **_store_views, const typename protocol_t::region_t &_region_mask) THROWS_NOTHING {
    for (int i = 0, e = store_views.size(); i < e; ++i) {
        rassert(store_views[i] == NULL);

        // We do a region intersection because store_subview_t requires that the region mask be a subset of the store region.
        store_views[i] = new store_subview_t<protocol_t>(_store_views[i], region_intersection(_region_mask, _store_views[i]->get_region()));
    }
}

template <class protocol_t>
multistore_ptr_t<protocol_t>::~multistore_ptr_t() {
    for (int i = 0, e = store_views.size(); i < e; ++i) {
        delete store_views[i];
    }
}

template <class protocol_t>
typename protocol_t::region_t multistore_ptr_t<protocol_t>::get_multistore_joined_region() const {
    store_view_t<protocol_t> *const *data = store_views.data();
    return compute_unmasked_multistore_joined_region(data, store_views.size());
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::new_read_tokens(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens_out,
                                                   int size) {
    guarantee(int(store_views.size()) == size);
    for (int i = 0; i < size; ++i) {
        store_views[i]->new_read_token(read_tokens_out[i]);
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::new_write_tokens(boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens_out,
                                                   int size) {
    guarantee(int(store_views.size()) == size);
    for (int i = 0; i < size; ++i) {
        store_views[i]->new_write_token(write_tokens_out[i]);
    }
}

template <class protocol_t>
region_map_t<protocol_t, version_range_t>  multistore_ptr_t<protocol_t>::
get_all_metainfos(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens, int num_read_tokens,
		  signal_t *interruptor) {

    guarantee(int(store_views.size()) == num_read_tokens);
    // TODO: Replace this with pmap, or have the caller do this
    // differently parallelly.

    region_map_t<protocol_t, version_range_t> ret(get_multistore_joined_region());
    for (int i = 0; i < num_read_tokens; ++i) {
	ret.update(region_map_transform<protocol_t, binary_blob_t, version_range_t>(store_views[i]->get_metainfo(read_tokens[i], interruptor),
										    &binary_blob_t::get<version_range_t>));
    }

    rassert(ret.get_domain() == region_mask);

    return ret;
}


template <class protocol_t>
typename protocol_t::region_t multistore_ptr_t<protocol_t>::get_region(int i) const {
    guarantee(0 <= i && i < num_stores());
    return store_views[i]->get_region();
}

template <class protocol_t>
store_view_t<protocol_t> *multistore_ptr_t<protocol_t>::get_store_view(int i) const {
    guarantee(0 <= i && i < num_stores());
    return store_views[i];
}



template <class protocol_t>
void multistore_ptr_t<protocol_t>::set_all_metainfos(const region_map_t<protocol_t, binary_blob_t> &new_metainfo, boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens, int num_write_tokens, signal_t *interruptor) {
    guarantee(num_write_tokens == num_stores());

    for (int i = 0; i < num_stores(); ++i) {
        store_views[i]->set_metainfo(new_metainfo.mask(store_views[i]->get_region()), write_tokens[i], interruptor);
    }
}

template <class protocol_t>
class multistore_send_backfill_should_backfill_t {
public:
    multistore_send_backfill_should_backfill_t(int num_stores, const typename protocol_t::region_t &joined_region,
                                               const boost::function<bool(const typename protocol_t::store_t::metainfo_t &)> &should_backfill_func)
        : countdown_(num_stores), should_backfill_func_(should_backfill_func), combined_metainfo_(joined_region) { }

    bool should_backfill(const typename protocol_t::store_t::metainfo_t &metainfo) {
        combined_metainfo_.update(metainfo);

        -- countdown_;
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
void multistore_ptr_t<protocol_t>::single_shard_backfill(int i,
                                                         multistore_send_backfill_should_backfill_t<protocol_t> *helper,
                                                         const region_map_t<protocol_t, state_timestamp_t> &start_point,
                                                         const boost::function<void(typename protocol_t::backfill_chunk_t)> &chunk_fun,
                                                         typename protocol_t::backfill_progress_t *progress,
                                                         boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                                                         signal_t *interruptor) THROWS_NOTHING {
    // TODO: This will be broken when things start being on multiple threads.

    // TODO: Blithely assing progress along might be broken.

    store_view_t<protocol_t> *store = store_views[i];
    try {
        store->send_backfill(start_point.mask(get_region(i)),
                             boost::bind(&multistore_send_backfill_should_backfill_t<protocol_t>::should_backfill, helper, _1),
                             chunk_fun,  // TODO: Do we need to wrap this?
                             progress,
                             read_tokens[i],
                             interruptor);
    } catch (interrupted_exc_t& exc) {
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
                                                            boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                                                            int num_stores_assertion,
                                                            signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    guarantee(num_stores() == num_stores_assertion);
    guarantee(region_is_superset(get_multistore_joined_region(), start_point.get_domain()));

    multistore_send_backfill_should_backfill_t<protocol_t> helper(num_stores(), get_multistore_joined_region(), should_backfill);

    pmap(num_stores(), boost::bind(&multistore_ptr_t<protocol_t>::single_shard_backfill, this, _1, &helper, boost::ref(start_point), boost::ref(chunk_fun), progress, read_tokens, interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    return helper.get_result();
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::single_shard_read(int i,
                                                     DEBUG_ONLY(const typename protocol_t::store_t::metainfo_t& expected_metainfo, )
                                                     const typename protocol_t::read_t &read,
                                                     boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                                                     std::vector<typename protocol_t::read_response_t> *responses,
                                                     signal_t *interruptor) THROWS_NOTHING {
    typename protocol_t::region_t ith_intersection = region_intersection(get_region(i), read.get_region());

    if (region_is_empty(ith_intersection)) {
        read_tokens[i].reset();
        return;
    }

    try {
        responses->push_back(store_views[i]->read(DEBUG_ONLY(expected_metainfo.mask(ith_intersection), )
                                                  read.shard(ith_intersection),
                                                  read_tokens[i],
                                                  interruptor));
    } catch (interrupted_exc_t& exc) {
        // do nothing
    }
}

template <class protocol_t>
typename protocol_t::read_response_t
multistore_ptr_t<protocol_t>::read(DEBUG_ONLY(const typename  protocol_t::store_t::metainfo_t& expected_metainfo, )
                                   const typename protocol_t::read_t &read,
                                   boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                                   int num_stores_assertion,
                                   signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    guarantee(num_stores() == num_stores_assertion);
    std::vector<typename protocol_t::read_response_t> responses;
    pmap(num_stores(), boost::bind(&multistore_ptr_t<protocol_t>::single_shard_read,
                                   this, _1, DEBUG_ONLY(boost::ref(expected_metainfo), )
                                   boost::ref(read),
                                   read_tokens,
                                   &responses,
                                   interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    typename protocol_t::temporary_cache_t fake_cache;
    return read.unshard(responses, &fake_cache);
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::single_shard_write(int i,
                                                      DEBUG_ONLY(const typename protocol_t::store_t::metainfo_t& expected_metainfo, )
                                                      const typename protocol_t::store_t::metainfo_t &new_metainfo,
                                                      const typename protocol_t::write_t &write,
                                                      transition_timestamp_t timestamp,
                                                      boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                                                      std::vector<typename protocol_t::write_response_t> *responses,
                                                      signal_t *interruptor) THROWS_NOTHING {
    typename protocol_t::region_t ith_intersection = region_intersection(get_region(i), write.get_region());
    if (region_is_empty(ith_intersection)) {
        write_tokens[i].reset();
        return;
    }

    // TODO: Have an assertion about the new_metainfo region?

    try {
        responses->push_back(store_views[i]->write(DEBUG_ONLY(expected_metainfo.mask(ith_intersection), )
                                                   new_metainfo.mask(ith_intersection),
                                                   write.shard(ith_intersection),
                                                   timestamp,
                                                   write_tokens[i],
                                                   interruptor));
    } catch (interrupted_exc_t& exc) {
        // do nothing
    }
}

template <class protocol_t>
typename protocol_t::write_response_t
multistore_ptr_t<protocol_t>::write(DEBUG_ONLY(const typename protocol_t::store_t::metainfo_t& expected_metainfo, )
                                    const typename protocol_t::store_t::metainfo_t& new_metainfo,
                                    const typename protocol_t::write_t &write,
                                    transition_timestamp_t timestamp,
                                    boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                                    int num_stores_assertion,
                                    signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    guarantee(num_stores() == num_stores_assertion);
    std::vector<typename protocol_t::write_response_t> responses;
    pmap(num_stores(), boost::bind(&multistore_ptr_t<protocol_t>::single_shard_write,
                                   this, _1, DEBUG_ONLY(boost::ref(expected_metainfo), )
                                   boost::ref(new_metainfo),
                                   boost::ref(write),
                                   timestamp,
                                   write_tokens,
                                   &responses,
                                   interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    typename protocol_t::temporary_cache_t fake_cache;
    return write.unshard(responses, &fake_cache);
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::single_shard_reset_all_data(int i,
                                                               const typename protocol_t::region_t &subregion,
                                                               const typename protocol_t::store_t::metainfo_t &new_metainfo,
                                                               boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                                                               signal_t *interruptor) THROWS_NOTHING {
    if (!region_overlaps(get_region(i), subregion)) {
        write_tokens[i].reset();
        return;
    }

    try {
        store_views[i]->reset_data(region_intersection(subregion, get_region(i)),
                                   new_metainfo.mask(get_region(i)),
                                   write_tokens[i],
                                   interruptor);
    } catch (interrupted_exc_t& exc) {
        // do nothing
    }
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::reset_all_data(const typename protocol_t::region_t &subregion,
                                                  const typename protocol_t::store_t::metainfo_t &new_metainfo,
                                                  boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                                                  int num_stores_assertion,
                                                  signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    guarantee(num_stores() == num_stores_assertion);
    pmap(num_stores(), boost::bind(&multistore_ptr_t<protocol_t>::single_shard_reset_all_data,
                                   this, _1,
                                   boost::ref(subregion),
                                   boost::ref(new_metainfo),
                                   write_tokens,
                                   interruptor));

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}








#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template class multistore_ptr_t<mock::dummy_protocol_t>;
template class multistore_ptr_t<memcached_protocol_t>;
