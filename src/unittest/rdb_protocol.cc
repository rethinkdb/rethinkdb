// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "buffer_cache/buffer_cache.hpp"
#include "clustering/administration/metadata.hpp"
#include "containers/iterators.hpp"
#include "extproc/pool.hpp"
#include "extproc/spawner.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/proto_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "unittest/dummy_namespace_interface.hpp"
#include "unittest/gtest.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace unittest {
namespace {

void run_with_namespace_interface(boost::function<void(namespace_interface_t<rdb_protocol_t> *, order_source_t *)> fun) {
    recreate_temporary_directory(base_path_t("."));

    order_source_t order_source;

    /* Pick shards */
    std::vector<rdb_protocol_t::region_t> shards;
    shards.push_back(rdb_protocol_t::region_t(key_range_t(key_range_t::none,   store_key_t(),  key_range_t::open, store_key_t("n"))));
    shards.push_back(rdb_protocol_t::region_t(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t() )));

    boost::ptr_vector<temp_file_t> temp_files;
    for (size_t i = 0; i < shards.size(); ++i) {
        temp_files.push_back(new temp_file_t);
    }

    scoped_ptr_t<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    scoped_array_t<scoped_ptr_t<serializer_t> > serializers(shards.size());
    for (size_t i = 0; i < shards.size(); ++i) {
        filepath_file_opener_t file_opener(temp_files[i].name(), io_backender.get());
        standard_serializer_t::create(&file_opener,
                                      standard_serializer_t::static_config_t());
        serializers[i].init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                                      &file_opener,
                                                      &get_global_perfmon_collection()));
    }

    boost::ptr_vector<rdb_protocol_t::store_t> underlying_stores;

    /* Create some structures for the rdb_protocol_t::context_t, warning some
     * boilerplate is about to follow, avert your eyes if you have a weak
     * stomach for such things. */
    extproc::spawner_info_t spawner_info;
    extproc::spawner_t::create(&spawner_info);
    extproc::pool_group_t pool_group(&spawner_info, extproc::pool_group_t::DEFAULTS);

    int port = randport();
    connectivity_cluster_t c;
    semilattice_manager_t<cluster_semilattice_metadata_t> slm(&c, cluster_semilattice_metadata_t());
    connectivity_cluster_t::run_t cr(&c, get_unittest_addresses(), port, &slm, 0, NULL);

    int port2 = randport();
    connectivity_cluster_t c2;
    directory_read_manager_t<cluster_directory_metadata_t> read_manager(&c2);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), port2, &read_manager, 0, NULL);

    rdb_protocol_t::context_t ctx(&pool_group, NULL, slm.get_root_view(), &read_manager, generate_uuid());

    for (size_t i = 0; i < shards.size(); ++i) {
        underlying_stores.push_back(
                new rdb_protocol_t::store_t(serializers[i].get(),
                    temp_files[i].name().permanent_path(), GIGABYTE, true,
                    &get_global_perfmon_collection(), &ctx,
                    io_backender.get(), base_path_t(".")));
    }

    boost::ptr_vector<store_view_t<rdb_protocol_t> > stores;
    for (size_t i = 0; i < shards.size(); ++i) {
        stores.push_back(new store_subview_t<rdb_protocol_t>(&underlying_stores[i], shards[i]));
    }

    /* Set up namespace interface */
    dummy_namespace_interface_t<rdb_protocol_t> nsi(shards, stores.c_array(), &order_source, &ctx);

    fun(&nsi, &order_source);
}

void run_in_thread_pool_with_namespace_interface(boost::function<void(namespace_interface_t<rdb_protocol_t> *, order_source_t*)> fun) {
    unittest::run_in_thread_pool(boost::bind(&run_with_namespace_interface, fun));
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
            ASSERT_EQ(maybe_point_write_response_t->result, STORED);
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
            ASSERT_TRUE(maybe_point_read_response->data->get() != NULL);
            ASSERT_TRUE(cJSON_Equal(data->get(), maybe_point_read_response->data->get()));
        } else {
            ADD_FAILURE() << "got wrong result back";
        }
    }
}

TEST(RDBProtocol, GetSet) {
    run_in_thread_pool_with_namespace_interface(&run_get_set_test);
}

void run_create_drop_sindex_test(namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *osource) {
    uuid_u id = generate_uuid();
    query_language::backtrace_t b;
    {
        /* Create a secondary index. */
        Term mapping;
        Term *arg = ql::pb::set_func(&mapping, 1);
        N2(GETATTR, NVAR(1), NDATUM("sid"));

        ql::map_wire_func_t m(mapping, static_cast<std::map<int64_t, Datum> *>(NULL));

        rdb_protocol_t::write_t write(rdb_protocol_t::sindex_create_t(id, m));
        rdb_protocol_t::write_response_t response;

        cond_t interruptor;
        nsi->write(write, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"), &interruptor);

        if (!boost::get<rdb_protocol_t::sindex_create_response_t>(&response.response)) {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    boost::shared_ptr<scoped_cJSON_t> data(new scoped_cJSON_t(cJSON_Parse("{\"id\" : 0, \"sid\" : 1}")));
    store_key_t pk = store_key_t(cJSON_print_primary(cJSON_GetObjectItem(data->get(), "id"), b));
    ASSERT_TRUE(data->get());
    {
        /* Insert a piece of data (it will be indexed using the secondary
         * index). */
        rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(pk, data));
        rdb_protocol_t::write_response_t response;

        cond_t interruptor;
        nsi->write(write, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"), &interruptor);

        if (rdb_protocol_t::point_write_response_t *maybe_point_write_response = boost::get<rdb_protocol_t::point_write_response_t>(&response.response)) {
            ASSERT_EQ(maybe_point_write_response->result, STORED);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        /* Access the data using the secondary index. */
        rdb_protocol_t::read_t read(rdb_protocol_t::rget_read_t(store_key_t(cJSON_print_primary(scoped_cJSON_t(cJSON_CreateNumber(1)).get(), b)), id));
        rdb_protocol_t::read_response_t response;

        cond_t interruptor;
        nsi->read(read, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"), &interruptor);

        if (rdb_protocol_t::rget_read_response_t *rget_resp = boost::get<rdb_protocol_t::rget_read_response_t>(&response.response)) {
            rdb_protocol_t::rget_read_response_t::stream_t *stream = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&rget_resp->result);
            ASSERT_TRUE(stream != NULL);
            ASSERT_TRUE(stream->size() == 1);
            ASSERT_TRUE(query_language::json_cmp(stream->at(0).second->get(), data->get()) == 0);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        /* Delete the data. */
        rdb_protocol_t::point_delete_t d(pk);
        rdb_protocol_t::write_t write(d);
        rdb_protocol_t::write_response_t response;

        cond_t interruptor;
        nsi->write(write, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"), &interruptor);

        if (rdb_protocol_t::point_delete_response_t *maybe_point_delete_response = boost::get<rdb_protocol_t::point_delete_response_t>(&response.response)) {
            ASSERT_EQ(maybe_point_delete_response->result, DELETED);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        /* Access the data using the secondary index. */
        rdb_protocol_t::read_t read(rdb_protocol_t::rget_read_t(store_key_t(cJSON_print_primary(scoped_cJSON_t(cJSON_CreateNumber(1)).get(), b)), id));
        rdb_protocol_t::read_response_t response;

        cond_t interruptor;
        nsi->read(read, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"), &interruptor);

        if (rdb_protocol_t::rget_read_response_t *rget_resp = boost::get<rdb_protocol_t::rget_read_response_t>(&response.response)) {
            rdb_protocol_t::rget_read_response_t::stream_t *stream = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&rget_resp->result);
            ASSERT_TRUE(stream != NULL);
            ASSERT_TRUE(stream->size() == 0);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        rdb_protocol_t::sindex_drop_t d(id);
        rdb_protocol_t::write_t write(d);
        rdb_protocol_t::write_response_t response;

        cond_t interruptor;
        nsi->write(write, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"), &interruptor);

        if (!boost::get<rdb_protocol_t::sindex_drop_response_t>(&response.response)) {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }
}

TEST(RDBProtocol, SindexCreateDrop) {
    run_in_thread_pool_with_namespace_interface(&run_create_drop_sindex_test);
}

}   /* namespace unittest */

