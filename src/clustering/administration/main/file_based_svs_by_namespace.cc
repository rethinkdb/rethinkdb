// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/main/file_based_svs_by_namespace.hpp"

#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/reactor/reactor.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "utils.hpp"

/* This object serves mostly as a container for arguments to the
 * do_construct_existing_store function because we hit the boost::bind argument
 * limit. */
template <class protocol_t>
struct store_args_t {
    store_args_t(io_backender_t *_io_backender, const base_path_t &_base_path,
            namespace_id_t _namespace_id, int64_t _cache_size,
            perfmon_collection_t *_serializers_perfmon_collection, typename
            protocol_t::context_t *_ctx)
        : io_backender(_io_backender), base_path(_base_path),
          namespace_id(_namespace_id), cache_size(_cache_size),
          serializers_perfmon_collection(_serializers_perfmon_collection),
          ctx(_ctx)
    { }

    io_backender_t *io_backender;
    base_path_t base_path;
    namespace_id_t namespace_id;
    int64_t cache_size;
    perfmon_collection_t *serializers_perfmon_collection;
    typename protocol_t::context_t *ctx;
};

std::string hash_shard_perfmon_name(int hash_shard_number) {
    return strprintf("shard_%d", hash_shard_number);
}

template <class protocol_t>
void do_construct_existing_store(
    const std::vector<threadnum_t> &threads,
    int thread_offset,
    store_args_t<protocol_t> store_args,
    serializer_multiplexer_t *multiplexer,
    scoped_array_t<scoped_ptr_t<typename protocol_t::store_t> > *stores_out_stores,
    store_view_t<protocol_t> **store_views) {

    // TODO: Exceptions?  Can exceptions happen, and then this doesn't
    // catch it, and the caller doesn't handle it.
    on_thread_t th(threads[thread_offset]);
    // TODO: Can we pass serializers_perfmon_collection across threads like this?
    typename protocol_t::store_t *store = new typename protocol_t::store_t(
        multiplexer->proxies[thread_offset], hash_shard_perfmon_name(thread_offset),
        store_args.cache_size, false, store_args.serializers_perfmon_collection,
        store_args.ctx, store_args.io_backender, store_args.base_path);
    (*stores_out_stores)[thread_offset].init(store);
    store_views[thread_offset] = store;
}

template <class protocol_t>
void do_create_new_store(
    const std::vector<threadnum_t> &threads,
    int thread_offset,
    store_args_t<protocol_t> store_args,
    serializer_multiplexer_t *multiplexer,
    scoped_array_t<scoped_ptr_t<typename protocol_t::store_t> > *stores_out_stores,
    store_view_t<protocol_t> **store_views) {

    on_thread_t th(threads[thread_offset]);
    typename protocol_t::store_t *store = new typename protocol_t::store_t(
        multiplexer->proxies[thread_offset], hash_shard_perfmon_name(thread_offset),
        store_args.cache_size, true, store_args.serializers_perfmon_collection,
        store_args.ctx, store_args.io_backender, store_args.base_path);
    (*stores_out_stores)[thread_offset].init(store);
    store_views[thread_offset] = store;
}

template <class protocol_t>
void
file_based_svs_by_namespace_t<protocol_t>::get_svs(
            perfmon_collection_t *serializers_perfmon_collection,
            namespace_id_t namespace_id,
            int64_t cache_size,
            stores_lifetimer_t<protocol_t> *stores_out,
            scoped_ptr_t<multistore_ptr_t<protocol_t> > *svs_out,
            typename protocol_t::context_t *ctx) {

    const int num_db_threads = get_num_db_threads();

    // TODO: If the server gets killed when starting up, we can
    // get a database in an invalid startup state.

    // TODO: This is quite suspicious in that we check if the file
    // exists and then assume it exists or does not exist when
    // loading or creating it.

    // TODO: We should use N slices on M serializers, not N slices
    // on N serializers.

    const int num_stores = CPU_SHARDING_FACTOR;
    scoped_array_t<scoped_ptr_t<typename protocol_t::store_t> > *stores_out_stores
        = stores_out->stores();
    stores_out_stores->init(num_stores);

    const threadnum_t serializer_thread = next_thread(num_db_threads);
    std::vector<threadnum_t> store_threads;
    for (int i = 0; i < num_stores; ++i) {
        store_threads.push_back(next_thread(num_db_threads));
    }

    scoped_ptr_t<standard_serializer_t> serializer;
    scoped_ptr_t<serializer_multiplexer_t> multiplexer;
    scoped_ptr_t<multistore_ptr_t<protocol_t> > mptr;
    {
        on_thread_t th(serializer_thread);
        scoped_array_t<store_view_t<protocol_t> *> store_views(num_stores);

        const serializer_filepath_t serializer_filepath = file_name_for(namespace_id);
        int res = access(serializer_filepath.permanent_path().c_str(), R_OK | W_OK);
        store_args_t<protocol_t> store_args(io_backender_, base_path_,
                                            namespace_id, cache_size,
                                            serializers_perfmon_collection, ctx);
        filepath_file_opener_t file_opener(serializer_filepath, io_backender_);
        if (res == 0) {
            // TODO: Could we handle failure when loading the serializer?  Right
            // now, we don't.
            serializer.init(new standard_serializer_t(
                                standard_serializer_t::dynamic_config_t(),
                                &file_opener,
                                serializers_perfmon_collection));

            std::vector<standard_serializer_t *> ptrs;
            ptrs.push_back(serializer.get());
            multiplexer.init(new serializer_multiplexer_t(ptrs));

            // TODO: Exceptions?  Can exceptions happen, and then
            // store_views' values would leak.  That is, are we handling
            // them in the pmap?  No.
            pmap(num_stores, boost::bind(do_construct_existing_store<protocol_t>,
                                         store_threads, _1, store_args,
                                         multiplexer.get(),
                                         stores_out_stores, store_views.data()));
            mptr.init(new multistore_ptr_t<protocol_t>(store_views.data(), num_stores));
        } else {
            standard_serializer_t::create(&file_opener,
                                          standard_serializer_t::static_config_t());
            serializer.init(new standard_serializer_t(
                                standard_serializer_t::dynamic_config_t(),
                                &file_opener,
                                serializers_perfmon_collection));

            std::vector<standard_serializer_t *> ptrs;
            ptrs.push_back(serializer.get());
            serializer_multiplexer_t::create(ptrs, num_stores);
            multiplexer.init(new serializer_multiplexer_t(ptrs));

            // TODO: How do we specify what the stores' regions are?
            // TODO: Exceptions?  Can exceptions happen, and then store_views'
            // values would leak.
            // The files do not exist, create them.
            // TODO: This should use pmap.
            pmap(num_stores, boost::bind(do_create_new_store<protocol_t>,
                                         store_threads, _1, store_args,
                                         multiplexer.get(),
                                         stores_out_stores, store_views.data()));
            mptr.init(new multistore_ptr_t<protocol_t>(store_views.data(), num_stores));

            // Initialize the metadata in the underlying stores.
            object_buffer_t<fifo_enforcer_sink_t::exit_write_t> write_token;
            mptr->new_write_token(&write_token);
            cond_t dummy_interruptor;
            order_source_t order_source;  // TODO: order_token_t::ignore.  Use the svs.
            guarantee(mptr->get_region() == protocol_t::region_t::universe());
            mptr->set_metainfo(
                region_map_t<protocol_t, binary_blob_t>(
                    mptr->get_region(),
                    binary_blob_t(version_range_t(version_t::zero()))),
                order_source.check_in("file_based_svs_by_namespace_t"),
                &write_token,
                &dummy_interruptor);

            // Finally, the store is created.
            file_opener.move_serializer_file_to_permanent_location();
        }
    } // back on calling thread

    svs_out->init(mptr.release());
    stores_out->serializer()->init(serializer.release());
    stores_out->multiplexer()->init(multiplexer.release());
}

template<class protocol_t>
void file_based_svs_by_namespace_t<protocol_t>::destroy_svs(namespace_id_t namespace_id) {
    // TODO: Handle errors?  It seems like we can't really handle the error so
    // let's just ignore it?
    const std::string filepath = file_name_for(namespace_id).permanent_path();
    const int res = ::unlink(filepath.c_str());
    guarantee_err(res == 0 || errno == ENOENT,
                  "unlink failed for file %s", filepath.c_str());
}

template<class protocol_t>
serializer_filepath_t file_based_svs_by_namespace_t<protocol_t>::file_name_for(namespace_id_t namespace_id) {
    return serializer_filepath_t(base_path_, uuid_to_str(namespace_id));
}

template<class protocol_t>
threadnum_t file_based_svs_by_namespace_t<protocol_t>::next_thread(int num_db_threads) {
    thread_counter_ = (thread_counter_ + 1) % num_db_threads;
    return threadnum_t(thread_counter_);
}

#include "mock/dummy_protocol.hpp"
template class file_based_svs_by_namespace_t<mock::dummy_protocol_t>;

#include "memcached/protocol.hpp"
template class file_based_svs_by_namespace_t<memcached_protocol_t>;

#include "rdb_protocol/protocol.hpp"
template class file_based_svs_by_namespace_t<rdb_protocol_t>;
