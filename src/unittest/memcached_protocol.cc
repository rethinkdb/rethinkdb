// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "memcached/protocol.hpp"
#include "buffer_cache/alt/cache_balancer.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "serializer/merger.hpp"
#include "unittest/gtest.hpp"
#include "unittest/dummy_namespace_interface.hpp"

namespace unittest {

void run_with_namespace_interface(boost::function<void(namespace_interface_t<memcached_protocol_t> *, order_source_t *)> fun) {

    order_source_t order_source;

    /* Pick shards */
    std::vector< hash_region_t<key_range_t> > shards;
    shards.push_back(hash_region_t<key_range_t>(key_range_t(key_range_t::none,   store_key_t(),  key_range_t::open, store_key_t("n"))));
    shards.push_back(hash_region_t<key_range_t>(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t() )));

    temp_file_t temp_file;

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);

    scoped_ptr_t<serializer_t> serializer;

    filepath_file_opener_t file_opener(temp_file.name(), &io_backender);
    standard_serializer_t::create(&file_opener,
                                  standard_serializer_t::static_config_t());

    serializer.init(new merger_serializer_t(
                        scoped_ptr_t<serializer_t>(
                            new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                                      &file_opener,
                                                      &get_global_perfmon_collection())),
                        MERGER_SERIALIZER_MAX_ACTIVE_WRITES));


    scoped_ptr_t<serializer_multiplexer_t> multiplexer;

    std::vector<serializer_t *> ptrs;
    ptrs.push_back(serializer.get());
    serializer_multiplexer_t::create(ptrs, shards.size());
    multiplexer.init(new serializer_multiplexer_t(ptrs));

    dummy_cache_balancer_t balancer(GIGABYTE);

    boost::ptr_vector<memcached_protocol_t::store_t> underlying_stores;
    for (size_t i = 0; i < shards.size(); ++i) {
        underlying_stores.push_back(
                new memcached_protocol_t::store_t(multiplexer->proxies[i], &balancer,
                    temp_file.name().permanent_path() + strprintf("_%zd", i),
                    true, &get_global_perfmon_collection(), NULL,
                    &io_backender, base_path_t(".")));
    }

    boost::ptr_vector<store_view_t<memcached_protocol_t> > stores;
    for (size_t i = 0; i < shards.size(); ++i) {
        stores.push_back(new store_subview_t<memcached_protocol_t>(&underlying_stores[i], shards[i]));
    }

    /* Set up namespace interface */
    memcached_protocol_t::context_t ctx;
    dummy_namespace_interface_t<memcached_protocol_t> nsi(shards,
            stores.c_array(), &order_source, &ctx);

    fun(&nsi, &order_source);
}

void run_in_thread_pool_with_namespace_interface(boost::function<void(namespace_interface_t<memcached_protocol_t> *, order_source_t *)> fun) {
    unittest::run_in_thread_pool(std::bind(&run_with_namespace_interface, fun));
}

/* `SetupTeardown` makes sure that it can start and stop without anything going
horribly wrong */
void run_setup_teardown_test(UNUSED namespace_interface_t<memcached_protocol_t> *nsi, UNUSED order_source_t *order_source) {
    /* Do nothing */
}
TEST(MemcachedProtocol, SetupTeardown) {
    run_in_thread_pool_with_namespace_interface(&run_setup_teardown_test);
}

/* `GetSet` tests basic get and set operations */
void run_get_set_test(namespace_interface_t<memcached_protocol_t> *nsi, order_source_t *order_source) {
    {
        sarc_mutation_t set;
        set.key = store_key_t("a");
        set.data = data_buffer_t::create(1);
        set.data->buf()[0] = 'A';
        set.flags = 123;
        set.exptime = 0;
        set.add_policy = add_policy_yes;
        set.replace_policy = replace_policy_yes;
        memcached_protocol_t::write_t write(set, time(NULL), 12345);

        cond_t interruptor;
        memcached_protocol_t::write_response_t result;
        nsi->write(write, &result, order_source->check_in("unittest::run_get_set_test(memcached_protocol.cc-A)"), &interruptor);

        if (set_result_t *maybe_set_result = boost::get<set_result_t>(&result.result)) {
            EXPECT_EQ(*maybe_set_result, sr_stored);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        get_query_t get;
        get.key = store_key_t("a");
        memcached_protocol_t::read_t read(get, time(NULL));

        cond_t interruptor;
        memcached_protocol_t::read_response_t result;
        nsi->read(read, &result, order_source->check_in("unittest::run_get_set_test(memcached_protocol.cc-B)").with_read_mode(), &interruptor);

        if (get_result_t *maybe_get_result = boost::get<get_result_t>(&result.result)) {
            EXPECT_TRUE(maybe_get_result->value.get() != NULL);
            EXPECT_EQ(1, maybe_get_result->value->size());
            if (maybe_get_result->value->size() == 1) {
                EXPECT_EQ('A', maybe_get_result->value->buf()[0]);
            }
            EXPECT_EQ(123u, maybe_get_result->flags);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        rget_query_t rget(hash_region_t<key_range_t>::universe(), 1000);
        memcached_protocol_t::read_t read(rget, time(NULL));

        cond_t interruptor;
        memcached_protocol_t::read_response_t result;
        nsi->read(read, &result, order_source->check_in("unittest::run_get_set_test(memcached_protocol.cc-C)").with_read_mode(), &interruptor);
        if (rget_result_t *maybe_rget_result = boost::get<rget_result_t>(&result.result)) {
            ASSERT_FALSE(maybe_rget_result->truncated);
            EXPECT_EQ(1u, maybe_rget_result->pairs.size());
            EXPECT_EQ(std::string("a"), key_to_unescaped_str(maybe_rget_result->pairs[0].key));
            EXPECT_EQ('A', maybe_rget_result->pairs[0].value_provider->buf()[0]);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }
}
TEST(MemcachedProtocol, GetSet) {
    run_in_thread_pool_with_namespace_interface(&run_get_set_test);
}

}   /* namespace unittest */

