#include "clustering/immediate_consistency/branch/multistore.hpp"

#include "protocol_api.hpp"
#include "rpc/semilattice/joins/vclock.hpp"

template <class protocol_t>
multistore_ptr_t<protocol_t>::multistore_ptr_t(const std::vector<store_view_t<protocol_t> *> &_store_views)
    : store_views(_store_views) {

    // do nothing?  For now.

}

template <class protocol_t>
multistore_ptr_t<protocol_t>::multistore_ptr_t(store_view_t<protocol_t> *_store_views, int num_store_views)
    : store_views(_store_views, _store_views + num_store_views) {

    // do nothing?  For now.

}


template <class protocol_t>
typename protocol_t::region_t multistore_ptr_t<protocol_t>::get_multistore_joined_region() const {
    std::vector<typename protocol_t::region_t> regions;
    for (int i = 0, e = store_views.size(); i < e; ++i) {
        regions.push_back(store_views[i]->get_region());
    }

    typename protocol_t::region_t reg;
    region_join_result_t res = region_join(regions, &reg);
    // TODO: This is what's supposed to happen, but is this really guaranteed?  (I think it is not.)
    guarantee(res == REGION_JOIN_OK);

    return reg;
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
void multistore_ptr_t<protocol_t>::get_all_metainfos(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens, int num_read_tokens,
						     signal_t *interruptor,
						     region_map_t<protocol_t, version_range_t> *metainfos_out, int num_metainfos) {
    guarantee(num_read_tokens == num_metainfos);
    guarantee(int(store_views.size()) == num_read_tokens);
    // TODO: Replace this with pmap, or have the caller do this
    // differently parallelly.

    for (int i = 0; i < num_read_tokens; ++i) {
	metainfos_out[i] = region_map_transform<protocol_t, binary_blob_t, version_range_t>(store_views[i]->get_metainfo(read_tokens[i], interruptor),
											    &binary_blob_t::get<version_range_t>);
    }
}









#include "memcached/protocol.hpp"
#include "mock/dummy_protocol.hpp"

template class multistore_ptr_t<mock::dummy_protocol_t>;
template class multistore_ptr_t<memcached_protocol_t>;
