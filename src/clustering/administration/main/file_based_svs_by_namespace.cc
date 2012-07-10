#include "clustering/administration/main/file_based_svs_by_namespace.hpp"

#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "db_thread_info.hpp"

template <class protocol_t>
void do_construct_existing_store(int i, io_backender_t *io_backender, perfmon_collection_t *serializers_perfmon_collection, const std::string& file_name_base,
                                 int num_db_threads, stores_lifetimer_t<protocol_t> *stores_out, store_view_t<protocol_t> **store_views) {

    // TODO: Exceptions?  Can exceptions happen, and then this doesn't
    // catch it, and the caller doesn't handle it.

    // TODO: A problem with this is that all the stores get
    // created on low-number threads.  What if there are
    // multiple namespaces?  We need a global
    // thread-distributor that evenly distributes stores about
    // threads.
    on_thread_t th(i % num_db_threads);

    const std::string file_name = file_name_base + "_" + strprintf("%d", i);

    // TODO: Can we pass serializers_perfmon_collection across threads like this?
    typename protocol_t::store_t *store = new typename protocol_t::store_t(io_backender, file_name, false, serializers_perfmon_collection);
    stores_out->stores()[i].reset(store);
    store_views[i] = store;
}

template <class protocol_t>
void do_create_new_store(int i, io_backender_t *io_backender, perfmon_collection_t *serializers_perfmon_collection, const std::string& file_name_base,
                         int num_db_threads, stores_lifetimer_t<protocol_t> *stores_out, store_view_t<protocol_t> **store_views) {
    // TODO: See the todo about thread distribution in do_construct_existing_store.  It is applicable here, too.
    on_thread_t th(i % num_db_threads);

    const std::string file_name = file_name_base + "_" + strprintf("%d", i);

    typename protocol_t::store_t *store = new typename protocol_t::store_t(io_backender, file_name, true, serializers_perfmon_collection);
    stores_out->stores()[i].reset(store);
    store_views[i] = store;
}

template <class protocol_t>
void
file_based_svs_by_namespace_t<protocol_t>::get_svs(perfmon_collection_t *serializers_perfmon_collection,
                                                   namespace_id_t namespace_id,
                                                   stores_lifetimer_t<protocol_t> *stores_out,
                                                   boost::scoped_ptr<multistore_ptr_t<protocol_t> > *svs_out) {

    const int num_db_threads = get_num_db_threads();

    // TODO: If the server gets killed when starting up, we can
    // get a database in an invalid startup state.
    const std::string file_name_base = file_path_ + "/" + uuid_to_str(namespace_id);

    // TODO: This is quite suspicious in that we check if the file
    // exists and then assume it exists or does not exist when
    // loading or creating it.

    // TODO: We should use N slices on M serializers, not N slices
    // on N serializers.

    int res = access((file_name_base + "_" + strprintf("%d", 0)).c_str(), R_OK | W_OK);
    if (res == 0) {
        int num_stores = 1;
        while (0 == access((file_name_base + "_" + strprintf("%d", num_stores)).c_str(), R_OK | W_OK)) {
            ++num_stores;
        }

        // The files already exist, thus we don't create them.
        boost::scoped_array<store_view_t<protocol_t> *> store_views(new store_view_t<protocol_t> *[num_stores]);
        stores_out->stores().reset(new boost::scoped_ptr<typename protocol_t::store_t>[num_stores]);
        stores_out->set_num_stores(num_stores);

        debugf("loading %d hash-sharded stores\n", num_stores);

        // TODO: Exceptions?  Can exceptions happen, and then
        // store_views' values would leak.  That is, are we handling
        // them in the pmap?  No.

        pmap(num_stores, boost::bind(do_construct_existing_store<protocol_t>, _1, io_backender_, serializers_perfmon_collection,
                                     boost::ref(file_name_base), num_db_threads, stores_out, store_views.get()));

        svs_out->reset(new multistore_ptr_t<protocol_t>(store_views.get(), num_stores));
    } else {
        const int num_stores = 4 + randint(4);
        debugf("creating %d hash-sharded stores\n", num_stores);
        stores_out->stores().reset(new boost::scoped_ptr<typename protocol_t::store_t>[num_stores]);
        stores_out->set_num_stores(num_stores);

        // TODO: How do we specify what the stores' regions are?

        // TODO: Exceptions?  Can exceptions happen, and then store_views' values would leak.

        // The files do not exist, create them.
        // TODO: This should use pmap.
        boost::scoped_array<store_view_t<protocol_t> *> store_views(new store_view_t<protocol_t> *[num_stores]);

        pmap(num_stores, boost::bind(do_create_new_store<protocol_t>, _1, io_backender_, serializers_perfmon_collection, boost::ref(file_name_base),
                                     num_db_threads, stores_out, store_views.get()));

        svs_out->reset(new multistore_ptr_t<protocol_t>(store_views.get(), num_stores));

        // Initialize the metadata in the underlying stores.
        scoped_ptr_t<fifo_enforcer_sink_t::exit_write_t> write_token;
        (*svs_out)->new_write_token(&write_token);

        cond_t dummy_interruptor;

        (*svs_out)->set_all_metainfos(region_map_t<protocol_t, binary_blob_t>((*svs_out)->get_multistore_joined_region(),
                                                                              binary_blob_t(version_range_t(version_t::zero()))),
                                      order_token_t::ignore,  // TODO
                                      &write_token,
                                      &dummy_interruptor);
    }
}

#include "mock/dummy_protocol.hpp"
template class file_based_svs_by_namespace_t<mock::dummy_protocol_t>;

#include "memcached/protocol.hpp"
template class file_based_svs_by_namespace_t<memcached_protocol_t>;
