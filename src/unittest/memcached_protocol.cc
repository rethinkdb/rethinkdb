#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "buffer_cache/buffer_cache.hpp"
#include "memcached/protocol.hpp"
#include "unittest/gtest.hpp"
#include "unittest/dummy_namespace_interface.hpp"

namespace unittest {

namespace {

void run_with_namespace_interface(boost::function<void(namespace_interface_t<memcached_protocol_t> *)> fun) {

    /* Pick shards */

    std::vector<key_range_t> shards;
    shards.push_back(key_range_t(key_range_t::none,   store_key_t(""),  key_range_t::open, store_key_t("n")));
    shards.push_back(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t("") ));

    /* Create temporary file */

    temp_file_t db_file("/tmp/rdb_unittest.XXXXXX");

    /* Set up serializer */

    standard_serializer_t::create(
        standard_serializer_t::dynamic_config_t(),
        standard_serializer_t::private_dynamic_config_t(db_file.name()),
        standard_serializer_t::static_config_t()
        );

    standard_serializer_t serializer(
        /* Extra parentheses are necessary so C++ doesn't interpret this as
        a declaration of a function called `serializer`. WTF, C++? */
        (standard_serializer_t::dynamic_config_t()),
        standard_serializer_t::private_dynamic_config_t(db_file.name())
        );

    /* Set up multiplexer */

    std::vector<standard_serializer_t *> multiplexer_files;
    multiplexer_files.push_back(&serializer);

    serializer_multiplexer_t::create(multiplexer_files, shards.size());

    serializer_multiplexer_t multiplexer(multiplexer_files);
    rassert(multiplexer.proxies.size() == shards.size());

    /* Set up caches, btrees, and stores */

    mirrored_cache_config_t cache_dynamic_config;
    boost::ptr_vector<cache_t> caches;
    boost::ptr_vector<btree_slice_t> btrees;
    std::vector<boost::shared_ptr<ready_store_view_t<memcached_protocol_t> > > stores;
    for (int i = 0; i < (int)shards.size(); i++) {
        mirrored_cache_static_config_t cache_static_config;
        cache_t::create(multiplexer.proxies[i], &cache_static_config);
        caches.push_back(new cache_t(multiplexer.proxies[i], &cache_dynamic_config));
        btree_slice_t::create(&caches[i]);
        btrees.push_back(new btree_slice_t(&caches[i]));
        stores.push_back(boost::make_shared<dummy_memcached_ready_store_view_t>(shards[i], &btrees[i]));
    }

    /* Set up namespace interface */

    dummy_namespace_interface_t<memcached_protocol_t> nsi(shards, stores);

    fun(&nsi);
}

void run_in_thread_pool_with_namespace_interface(boost::function<void(namespace_interface_t<memcached_protocol_t> *)> fun) {
    run_in_thread_pool(boost::bind(&run_with_namespace_interface, fun));
}

}   /* anonymous namespace */

/* `SetupTeardown` makes sure that it can start and stop without anything going
horribly wrong */

void run_setup_teardown_test(UNUSED namespace_interface_t<memcached_protocol_t> *nsi) {
    /* Do nothing */
}
TEST(MemcachedProtocol, SetupTeardown) {
    run_in_thread_pool_with_namespace_interface(&run_setup_teardown_test);
}

/* `GetSet` tests basic get and set operations */

void run_get_set_test(namespace_interface_t<memcached_protocol_t> *nsi) {

    order_source_t osource;

    {
        sarc_mutation_t set;
        set.key = store_key_t("a");
        set.data = data_buffer_t::create(1);
        set.data->buf()[0] = 'A';
        set.flags = 123;
        set.exptime = 0;
        set.add_policy = add_policy_yes;
        set.replace_policy = replace_policy_yes;
        memcached_protocol_t::write_t write(set, 12345);

        cond_t interruptor;
        memcached_protocol_t::write_response_t result = nsi->write(write, osource.check_in("unittest"), &interruptor);

        if (set_result_t *maybe_set_result = boost::get<set_result_t>(&result.result)) {
            EXPECT_EQ(*maybe_set_result, sr_stored);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        get_query_t get;
        get.key = store_key_t("a");
        memcached_protocol_t::read_t read(get);

        cond_t interruptor;
        memcached_protocol_t::read_response_t result = nsi->read(read, osource.check_in("unittest"), &interruptor);

        if (get_result_t *maybe_get_result = boost::get<get_result_t>(&result.result)) {
            EXPECT_FALSE(maybe_get_result->is_not_allowed);
            EXPECT_TRUE(maybe_get_result->value.get() != NULL);
            EXPECT_EQ(1, maybe_get_result->value->size());
            if (maybe_get_result->value->size() == 1) {
                EXPECT_EQ('A', maybe_get_result->value->buf()[0]);
            }
            EXPECT_EQ(123, maybe_get_result->flags);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }
}
TEST(MemcachedProtocol, GetSet) {
    run_in_thread_pool_with_namespace_interface(&run_get_set_test);
}

}   /* namespace unittest */
