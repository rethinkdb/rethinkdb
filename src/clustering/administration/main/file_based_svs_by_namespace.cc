#include "clustering/administration/main/file_based_svs_by_namespace.hpp"

#include "clustering/immediate_consistency/branch/multistore.hpp"

template <class protocol_t>
void
file_based_svs_by_namespace_t<protocol_t>::get_svs(
                perfmon_collection_t *perfmon_collection,
                namespace_id_t namespace_id,
                boost::scoped_array<boost::scoped_ptr<typename protocol_t::store_t> > *stores_out,
                boost::scoped_ptr<multistore_ptr_t<protocol_t> > *svs_out) {

    // TODO: If the server gets killed when starting up, we can
    // get a database in an invalid startup state.

    // TODO: Obviously, the hard-coded numeric constant here might
    // be regarded as a problem.  Randomly choosing between 4 or 5
    // is pretty cool though.
    const std::string file_name_base = file_path_ + "/" + uuid_to_str(namespace_id);

    // TODO: This is quite suspicious in that we check if the file
    // exists and then assume it exists or does not exist when
    // loading or creating it.

    // TODO: Also we only check file 0.

    // TODO: We should use N slices on M serializers, not N slices
    // on N serializers.

    //        stores_out->reset(new boost::scoped_ptr<typename protocol_t::store_t>[num_stores]);

    int res = access((file_name_base + "_" + strprintf("%d", 0)).c_str(), R_OK | W_OK);
    if (res == 0) {
        int num_stores = 1;
        while (0 == access((file_name_base + "_" + strprintf("%d", num_stores)).c_str(), R_OK | W_OK)) {
            ++num_stores;
        }

        // The files already exist, thus we don't create them.
        boost::scoped_array<store_view_t<protocol_t> *> store_views(new store_view_t<protocol_t> *[num_stores]);
        stores_out->reset(new boost::scoped_ptr<typename protocol_t::store_t>[num_stores]);

        debugf("loading %d hash-sharded stores\n", num_stores);

        // TODO: Exceptions?  Can exceptions happen, and then store_views' values would leak.

        // TODO: This should use pmap.
        for (int i = 0; i < num_stores; ++i) {
            const std::string file_name = file_name_base + "_" + strprintf("%d", i);
            (*stores_out)[i].reset(new typename protocol_t::store_t(file_name, false, perfmon_collection));
            store_views[i] = (*stores_out)[i].get();
        }

        svs_out->reset(new multistore_ptr_t<protocol_t>(store_views.get(), num_stores));
    } else {
        // num_stores randomization is commented out to simplify experiments with figuring out what's wrong with rebalance.
        const int num_stores = 4 + randint(4);
        debugf("creating %d hash-sharded stores\n", num_stores);
        stores_out->reset(new boost::scoped_ptr<typename protocol_t::store_t>[num_stores]);

        // TODO: How do we specify what the stores' regions are?

        // TODO: Exceptions?  Can exceptions happen, and then store_views' values would leak.

        // The files do not exist, create them.
        // TODO: This should use pmap.
        boost::scoped_array<store_view_t<protocol_t> *> store_views(new store_view_t<protocol_t> *[num_stores]);
        for (int i = 0; i < num_stores; ++i) {
            const std::string file_name = file_name_base + "_" + strprintf("%d", i);
            (*stores_out)[i].reset(new typename protocol_t::store_t(file_name, true, perfmon_collection));
            store_views[i] = (*stores_out)[i].get();
        }

        svs_out->reset(new multistore_ptr_t<protocol_t>(store_views.get(), num_stores));

        // Initialize the metadata in the underlying stores.
        boost::scoped_array<boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> > write_tokens(new boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t>[num_stores]);

        (*svs_out)->new_write_tokens(write_tokens.get(), num_stores);

        cond_t dummy_interruptor;

        (*svs_out)->set_all_metainfos(region_map_t<protocol_t, binary_blob_t>((*svs_out)->get_multistore_joined_region(),
                                                                              binary_blob_t(version_range_t(version_t::zero()))),
                                      order_token_t::ignore,  // TODO
                                      write_tokens.get(),
                                      num_stores,
                                      &dummy_interruptor);
    }
}

template <class protocol_t>
void file_based_svs_by_namespace_t<protocol_t>::destroy_svs(namespace_id_t namespace_id) {
    (void)namespace_id;
}

#include "mock/dummy_protocol.hpp"
template class file_based_svs_by_namespace_t<mock::dummy_protocol_t>;

#include "memcached/protocol.hpp"
template class file_based_svs_by_namespace_t<memcached_protocol_t>;
