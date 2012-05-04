#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_

#include <vector>

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "concurrency/fifo_enforcer.hpp"

template <class> class store_view_t;
template <class, class> class region_map_t;
class version_range_t;
class binary_blob_t;

template <class protocol_t>
class multistore_ptr_t {
public:
    // We don't get ownership of the store_view_t pointers themselves.
    multistore_ptr_t(const std::vector<store_view_t<protocol_t> *> &store_views);

    // We don't get ownership of the store_view_t pointers themselves.
    multistore_ptr_t(store_view_t<protocol_t> *store_views, int num_store_views);

    typename protocol_t::region_t get_multistore_joined_region() const;

    int num_stores() const { return store_views.size(); }

    void new_read_tokens(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens_out, int size_assertion);

    void new_write_tokens(boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens_out, int size_assertion);

    // TODO: Make this operate on binary_blob_t.
    region_map_t<protocol_t, version_range_t>
    get_all_metainfos(boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> *read_tokens, int num_read_tokens,
		      signal_t *interruptor);

    typename protocol_t::region_t get_region(int i) const;
    store_view_t<protocol_t> *get_store_view(int i) const;

    // This is the opposite of get_all_metainfos but is a bit more scary.
    void set_all_metainfos(const region_map_t<protocol_t, binary_blob_t> &new_metainfo, boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> *write_tokens, int num_write_tokens, signal_t *interruptor);


private:
    // We don't own these pointers.
    const std::vector<store_view_t<protocol_t> *> store_views;

    DISABLE_COPYING(multistore_ptr_t);
};




#endif  // CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_MULTISTORE_HPP_

