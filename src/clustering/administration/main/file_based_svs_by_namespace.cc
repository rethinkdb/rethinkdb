// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/main/file_based_svs_by_namespace.hpp"

#include "clustering/immediate_consistency/branch/multistore.hpp"
#include "clustering/reactor/reactor.hpp"
#include "db_thread_info.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"

/* This object serves mostly as a container for arguments to the
 * do_construct_existing_store function because we hit the boost::bind argument
 * limit. */
template <class protocol_t>
struct store_args_t {
    store_args_t(io_backender_t *_io_backender, namespace_id_t _namespace_id,
                 int64_t _cache_size, perfmon_collection_t *_serializers_perfmon_collection,
                 typename protocol_t::context_t *_ctx)
        : io_backender(_io_backender), namespace_id(_namespace_id), cache_size(_cache_size),
          serializers_perfmon_collection(_serializers_perfmon_collection),
          ctx(_ctx)
    { }

    io_backender_t *io_backender;
    namespace_id_t namespace_id;
    int64_t cache_size;
    perfmon_collection_t *serializers_perfmon_collection;
    typename protocol_t::context_t *ctx;
};

std::string hash_shard_perfmon_name(int hash_shard_number) {
    return strprintf("shard_%d", hash_shard_number);
}

template <class protocol_t>
void do_construct_existing_store(int i,
                                 store_args_t<protocol_t> store_args,
                                 serializer_multiplexer_t *multiplexer,
                                 int num_db_threads,
                                 stores_lifetimer_t<protocol_t> *stores_out,
                                 store_view_t<protocol_t> **store_views) {

    // TODO: Exceptions?  Can exceptions happen, and then this doesn't
    // catch it, and the caller doesn't handle it.

    // TODO: A problem with this is that all the stores get
    // created on low-number threads.  What if there are
    // multiple namespaces?  We need a global
    // thread-distributor that evenly distributes stores about
    // threads.
    on_thread_t th(i % num_db_threads);

    // TODO: Can we pass serializers_perfmon_collection across threads like this?
    typename protocol_t::store_t *store = new typename protocol_t::store_t(multiplexer->proxies[i], hash_shard_perfmon_name(i),
                                                                           store_args.cache_size, false, store_args.serializers_perfmon_collection,
                                                                           store_args.ctx);
    (*stores_out->stores())[i].init(store);
    store_views[i] = store;
}

template <class protocol_t>
void do_create_new_store(int i,
                         store_args_t<protocol_t> store_args,
                         serializer_multiplexer_t *multiplexer,
                         int num_db_threads,
                         stores_lifetimer_t<protocol_t> *stores_out,
                         store_view_t<protocol_t> **store_views) {
    // TODO: See the todo about thread distribution in do_construct_existing_store.  It is applicable here, too.
    on_thread_t th(i % num_db_threads);

    typename protocol_t::store_t *store = new typename protocol_t::store_t(multiplexer->proxies[i], hash_shard_perfmon_name(i),
                                                                           store_args.cache_size, true, store_args.serializers_perfmon_collection,
                                                                           store_args.ctx);
    (*stores_out->stores())[i].init(store);
    store_views[i] = store;
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

    const int num_stores = CLUSTER_CPU_SHARDING_FACTOR;

    const std::string serializer_filepath = file_name_for(namespace_id);

    scoped_ptr_t<standard_serializer_t> serializer;
    scoped_ptr_t<serializer_multiplexer_t> multiplexer;

    int res = access(serializer_filepath.c_str(), R_OK | W_OK);
    store_args_t<protocol_t> store_args(io_backender_, namespace_id, cache_size, serializers_perfmon_collection, ctx);
    if (res == 0) {
        // TODO: Could we handle failure when loading the serializer?  Right now, we don't.
        serializer.init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                                  io_backender_,
                                                  standard_serializer_t::private_dynamic_config_t(serializer_filepath),
                                                  serializers_perfmon_collection));

        std::vector<standard_serializer_t *> ptrs;
        ptrs.push_back(serializer.get());
        multiplexer.init(new serializer_multiplexer_t(ptrs));

        // The files already exist, thus we don't create them.
        scoped_array_t<store_view_t<protocol_t> *> store_views(num_stores);
        stores_out->stores()->init(num_stores);


        // TODO: Exceptions?  Can exceptions happen, and then
        // store_views' values would leak.  That is, are we handling
        // them in the pmap?  No.

        pmap(num_stores, boost::bind(do_construct_existing_store<protocol_t>,
                                     _1, store_args, multiplexer.get(),
                                     num_db_threads, stores_out, store_views.data()));

        svs_out->init(new multistore_ptr_t<protocol_t>(store_views.data(), num_stores));
    } else {
        stores_out->stores()->init(num_stores);

	filepath_file_opener_t file_opener(serializer_filepath, io_backender_);
        standard_serializer_t::create(&file_opener,
                                      standard_serializer_t::static_config_t());

        serializer.init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                                  io_backender_,
                                                  standard_serializer_t::private_dynamic_config_t(serializer_filepath),
                                                  serializers_perfmon_collection));

        std::vector<standard_serializer_t *> ptrs;
        ptrs.push_back(serializer.get());
        serializer_multiplexer_t::create(ptrs, num_stores);
        multiplexer.init(new serializer_multiplexer_t(ptrs));


        // TODO: How do we specify what the stores' regions are?

        // TODO: Exceptions?  Can exceptions happen, and then store_views' values would leak.

        // The files do not exist, create them.
        // TODO: This should use pmap.
        scoped_array_t<store_view_t<protocol_t> *> store_views(num_stores);

        pmap(num_stores, boost::bind(do_create_new_store<protocol_t>,
                                     _1,  store_args, multiplexer.get(),
                                     num_db_threads, stores_out, store_views.data()));

        svs_out->init(new multistore_ptr_t<protocol_t>(store_views.data(), num_stores));

        // Initialize the metadata in the underlying stores.
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t> write_token;
        (*svs_out)->new_write_token(&write_token);

        cond_t dummy_interruptor;

        order_source_t order_source;  // TODO: order_token_t::ignore.  Use the svs.

        guarantee((*svs_out)->get_region() == protocol_t::region_t::universe());
        (*svs_out)->set_metainfo(region_map_t<protocol_t, binary_blob_t>((*svs_out)->get_region(),
                                                                         binary_blob_t(version_range_t(version_t::zero()))),
                                 order_source.check_in("file_based_svs_by_namespace_t"),
                                 &write_token,
                                 &dummy_interruptor);
    }

    stores_out->serializer()->init(serializer.release());
    stores_out->multiplexer()->init(multiplexer.release());
}

template <class protocol_t>
void file_based_svs_by_namespace_t<protocol_t>::destroy_svs(namespace_id_t namespace_id) {
    // TODO: Handle errors?  It seems like we can't really handle the error so let's just ignore it?
    unlink(file_name_for(namespace_id).c_str());
}

template <class protocol_t>
std::string file_based_svs_by_namespace_t<protocol_t>::file_name_for(namespace_id_t namespace_id) {
    return file_path_ + "/" + uuid_to_str(namespace_id);
}

template <class protocol_t>
std::string file_based_svs_by_namespace_t<protocol_t>::hash_shard_name_for(namespace_id_t namespace_id, int shardnum) {
    return file_name_for(namespace_id) + strprintf("_%d", shardnum);
}

#include "mock/dummy_protocol.hpp"
template class file_based_svs_by_namespace_t<mock::dummy_protocol_t>;

#include "memcached/protocol.hpp"
template class file_based_svs_by_namespace_t<memcached_protocol_t>;

#include "rdb_protocol/protocol.hpp"
template class file_based_svs_by_namespace_t<rdb_protocol_t>;
