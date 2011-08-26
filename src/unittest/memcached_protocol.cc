#include "unittest/gtest.hpp"
#include "memcached/protocol.hpp"
#include "unittest/dummy_namespace_interface.hpp"
#include <boost/make_shared.hpp>

namespace unittest {

namespace {

void run_with_namespace_interface(boost::function<void(namespace_interface_t<memcached_protocol_t> *)> fun) {

    std::vector<key_range_t> shards;
    shards.push_back(key_range_t(key_range_t::none,   store_key_t(""),  key_range_t::open, store_key_t("n")));
    shards.push_back(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t("") ));
    const int repli_factor = 3;

    run_with_dummy_namespace_interface<memcached_protocol_t>(shards, repli_factor, fun);
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

        memcached_protocol_t::write_response_t result = nsi->write(write, osource.check_in("unittest"));

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

        memcached_protocol_t::read_response_t result = nsi->read(read, osource.check_in("unittest"));

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
