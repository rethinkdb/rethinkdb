// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "buffer_cache/buffer_cache.hpp"
#include "containers/iterators.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "unittest/gtest.hpp"
#include "unittest/dummy_namespace_interface.hpp"

namespace unittest {
namespace {

void run_with_namespace_interface(boost::function<void(namespace_interface_t<rdb_protocol_t> *, order_source_t *)> fun) {

    order_source_t order_source;

    /* Pick shards */
    std::vector<rdb_protocol_t::region_t> shards;
    shards.push_back(rdb_protocol_t::region_t(key_range_t(key_range_t::none,   store_key_t(),  key_range_t::open, store_key_t("n"))));
    shards.push_back(rdb_protocol_t::region_t(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t() )));

    boost::ptr_vector<mock::temp_file_t> temp_files;
    for (size_t i = 0; i < shards.size(); ++i) {
        temp_files.push_back(new mock::temp_file_t("/tmp/rdb_unittest.XXXXXX"));
    }

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    scoped_array_t<scoped_ptr_t<serializer_t> > serializers(shards.size());
    for (size_t i = 0; i < shards.size(); ++i) {
	filepath_file_opener_t file_opener(temp_files[i].name(), io_backender.get());
        standard_serializer_t::create(&file_opener,
                                      standard_serializer_t::static_config_t());
        serializers[i].init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                                      io_backender.get(),
                                                      standard_serializer_t::private_dynamic_config_t(temp_files[i].name()),
                                                      &get_global_perfmon_collection()));
    }

    boost::ptr_vector<rdb_protocol_t::store_t> underlying_stores;
    rdb_protocol_t::context_t ctx;

    for (size_t i = 0; i < shards.size(); ++i) {
        underlying_stores.push_back(new rdb_protocol_t::store_t(serializers[i].get(), temp_files[i].name(), GIGABYTE, true, &get_global_perfmon_collection(), &ctx));
    }

    boost::ptr_vector<store_view_t<rdb_protocol_t> > stores;
    for (size_t i = 0; i < shards.size(); ++i) {
        stores.push_back(new store_subview_t<rdb_protocol_t>(&underlying_stores[i], shards[i]));
    }

    /* Set up namespace interface */
    dummy_namespace_interface_t<rdb_protocol_t> nsi(shards, stores.c_array(), &order_source);

    fun(&nsi, &order_source);
}

void run_in_thread_pool_with_namespace_interface(boost::function<void(namespace_interface_t<rdb_protocol_t> *, order_source_t*)> fun) {
    mock::run_in_thread_pool(boost::bind(&run_with_namespace_interface, fun));
}

}   /* anonymous namespace */

/* `SetupTeardown` makes sure that it can start and stop without anything going
horribly wrong */
void run_setup_teardown_test(UNUSED namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *) {
    /* Do nothing */
}
TEST(RDBProtocol, SetupTeardown) {
    run_in_thread_pool_with_namespace_interface(&run_setup_teardown_test);
}

/* `GetSet` tests basic get and set operations */
void run_get_set_test(namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *osource) {
    boost::shared_ptr<scoped_cJSON_t> data(new scoped_cJSON_t(cJSON_CreateNull()));
    {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t("a"), data));
        rdb_protocol_t::write_response_t response;

        cond_t interruptor;
        nsi->write(write, &response, osource->check_in("unittest::run_get_set_test(rdb_protocol.cc-A)"), &interruptor);

        if (rdb_protocol_t::point_write_response_t *maybe_point_write_response_t = boost::get<rdb_protocol_t::point_write_response_t>(&response.response)) {
            EXPECT_EQ(maybe_point_write_response_t->result, STORED);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        rdb_protocol_t::read_t read(rdb_protocol_t::point_read_t(store_key_t("a")));
        rdb_protocol_t::read_response_t response;

        cond_t interruptor;
        nsi->read(read, &response, osource->check_in("unittest::run_get_set_test(rdb_protocol.cc-B)"), &interruptor);

        if (rdb_protocol_t::point_read_response_t *maybe_point_read_response = boost::get<rdb_protocol_t::point_read_response_t>(&response.response)) {
            EXPECT_TRUE(maybe_point_read_response->data->get() != NULL);
            EXPECT_TRUE(cJSON_Equal(data->get(), maybe_point_read_response->data->get()));
        } else {
            ADD_FAILURE() << "got wrong result back";
        }
    }
}

//TEST(RDBProtocol, GetSet) {
//    run_in_thread_pool_with_namespace_interface(&run_get_set_test);
//}

}   /* namespace unittest */

