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
    : store_views(num_store_views, NULL), region_mask(_region_mask) {

    rassert(region_is_superset(compute_unmasked_multistore_joined_region(_store_views, num_store_views), _region_mask));

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
bool multistore_ptr_t<protocol_t>::send_multistore_backfill(UNUSED const region_map_t<protocol_t, state_timestamp_t> &start_point,
                                                            UNUSED const boost::function<bool(const typename protocol_t::store_t::metainfo_t &)> &should_backfill,
                                                            UNUSED const boost::function<void(typename protocol_t::backfill_chunk_t)> &chunk_fun,
                                                            UNUSED typename protocol_t::backfill_progress_t *progress,
                                                            UNUSED boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                                                            UNUSED int num_stores_assertion,
                                                            UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    // We need to acquire all the superblocks of all the stores.  Then
    // we need to get and join all their metainfo.  Then we need to
    // call should_backfill(metainfo).

    // TODO: All the parameters must be not marked UNUSED.

    // TODO: uh, no.
    return false;

}



template <class protocol_t>
typename protocol_t::read_response_t
multistore_ptr_t<protocol_t>::read(DEBUG_ONLY(UNUSED const typename  protocol_t::store_t::metainfo_t& expected_metainfo, )
                                   UNUSED const typename protocol_t::read_t &read,
                                   UNUSED boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens,
                                   UNUSED int num_stores_assertion,
                                   UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    // TODO: uh, implement.

    // TODO: no unused parameters.

    return typename protocol_t::read_response_t();

}

template <class protocol_t>
typename protocol_t::write_response_t
multistore_ptr_t<protocol_t>::write(DEBUG_ONLY(UNUSED const typename protocol_t::store_t::metainfo_t& expected_metainfo, )
                                    UNUSED const typename protocol_t::store_t::metainfo_t& new_metainfo,
                                    UNUSED const typename protocol_t::write_t &write,
                                    UNUSED transition_timestamp_t timestamp,
                                    UNUSED boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                                    UNUSED int num_stores_assertion,
                                    UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    // TODO: uh, implement
    // TODO: no unused parameters.

    return typename protocol_t::write_response_t();
}

template <class protocol_t>
void multistore_ptr_t<protocol_t>::reset_all_data(UNUSED typename protocol_t::region_t subregion,
                                                  UNUSED const typename protocol_t::store_t::metainfo_t &new_metainfo,
                                                  UNUSED boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens,
                                                  UNUSED int num_stores_assertion,
                                                  UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    // todo: uh, implement
    // TODO: no unused parameters.

}








#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template class multistore_ptr_t<mock::dummy_protocol_t>;
template class multistore_ptr_t<memcached_protocol_t>;
