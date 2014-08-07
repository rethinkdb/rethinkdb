// Copyright 2010-2014 RethinkDB, all rights reserved.
#include <vector>

#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "arch/io/disk.hpp"
#include "buffer_cache/alt/cache_balancer.hpp"
#include "clustering/administration/metadata.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/store.hpp"
#include "region/region_json_adapter.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "stl_utils.hpp"
#include "unittest/dummy_namespace_interface.hpp"
#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {
namespace {

void run_with_namespace_interface(
        boost::function<void(namespace_interface_t *, order_source_t *)> fun,
        bool oversharding,
        int num_restarts) {
    recreate_temporary_directory(base_path_t("."));

    /* Pick shards */
    std::vector<region_t> store_shards;
    if (oversharding) {
        store_shards.push_back(region_t(key_range_t(key_range_t::none, store_key_t(), key_range_t::none, store_key_t())));
    } else {
        store_shards.push_back(region_t(key_range_t(key_range_t::none, store_key_t(), key_range_t::open, store_key_t("n"))));
        store_shards.push_back(region_t(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t() )));
    }

    std::vector<region_t> nsi_shards;

    nsi_shards.push_back(region_t(key_range_t(key_range_t::none,   store_key_t(),  key_range_t::open, store_key_t("n"))));
    nsi_shards.push_back(region_t(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t() )));

    std::vector<scoped_ptr_t<temp_file_t> > temp_files;
    for (size_t i = 0; i < store_shards.size(); ++i) {
        temp_files.push_back(make_scoped<temp_file_t>());
    }

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);
    dummy_cache_balancer_t balancer(GIGABYTE);

    scoped_array_t<scoped_ptr_t<serializer_t> > serializers(store_shards.size());
    for (size_t i = 0; i < store_shards.size(); ++i) {
        filepath_file_opener_t file_opener(temp_files[i]->name(), &io_backender);
        standard_serializer_t::create(&file_opener,
                                      standard_serializer_t::static_config_t());
        serializers[i].init(new standard_serializer_t(standard_serializer_t::dynamic_config_t(),
                                                      &file_opener,
                                                      &get_global_perfmon_collection()));
    }

    extproc_pool_t extproc_pool(2);
    rdb_context_t ctx(&extproc_pool, NULL);

    for (int rep = 0; rep < num_restarts; ++rep) {
        const bool do_create = rep == 0;
        std::vector<scoped_ptr_t<store_t> > underlying_stores;
        for (size_t i = 0; i < store_shards.size(); ++i) {
            underlying_stores.push_back(
                    make_scoped<store_t>(serializers[i].get(), &balancer,
                        temp_files[i]->name().permanent_path(), do_create,
                        &get_global_perfmon_collection(), &ctx,
                        &io_backender, base_path_t(".")));
        }

        std::vector<scoped_ptr_t<store_view_t> > stores;
        std::vector<store_view_t *> store_ptrs;
        for (size_t i = 0; i < nsi_shards.size(); ++i) {
            if (oversharding) {
                stores.push_back(make_scoped<store_subview_t>(underlying_stores[0].get(),
                                                              nsi_shards[i]));
            } else {
                stores.push_back(make_scoped<store_subview_t>(underlying_stores[i].get(),
                                                              nsi_shards[i]));
            }
            store_ptrs.push_back(stores.back().get());
        }

        /* Set up namespace interface */
        order_source_t order_source;
        dummy_namespace_interface_t nsi(nsi_shards,
                                        store_ptrs.data(),
                                        &order_source,
                                        &ctx,
                                        do_create);

        fun(&nsi, &order_source);
    }
}

void run_in_thread_pool_with_namespace_interface(
        boost::function<void(namespace_interface_t *, order_source_t*)> fun,
        bool oversharded,
        int num_restarts = 1) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(std::bind(&run_with_namespace_interface,
                                           fun,
                                           oversharded,
                                           num_restarts));
}

}   /* anonymous namespace */

/* `SetupTeardown` makes sure that it can start and stop without anything going
horribly wrong */
void run_setup_teardown_test(UNUSED namespace_interface_t *nsi, order_source_t *) {
    /* Do nothing */
}
TEST(RDBProtocol, SetupTeardown) {
    run_in_thread_pool_with_namespace_interface(&run_setup_teardown_test, false);
}

TEST(RDBProtocol, OvershardedSetupTeardown) {
    run_in_thread_pool_with_namespace_interface(&run_setup_teardown_test, true);
}

/* `GetSet` tests basic get and set operations */
void run_get_set_test(namespace_interface_t *nsi, order_source_t *osource) {
    {
        write_t write(
                point_write_t(store_key_t("a"), ql::datum_t::null()),
                DURABILITY_REQUIREMENT_DEFAULT,
                profile_bool_t::PROFILE,
                ql::configured_limits_t());
        write_response_t response;

        cond_t interruptor;
        nsi->write(write, &response, osource->check_in("unittest::run_get_set_test(rdb_protocol.cc-A)"), &interruptor);

        if (point_write_response_t *maybe_point_write_response_t = boost::get<point_write_response_t>(&response.response)) {
            ASSERT_EQ(maybe_point_write_response_t->result, point_write_result_t::STORED);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        read_t read(point_read_t(store_key_t("a")), profile_bool_t::PROFILE);
        read_response_t response;

        cond_t interruptor;
        nsi->read(read, &response, osource->check_in("unittest::run_get_set_test(rdb_protocol.cc-B)"), &interruptor);

        if (point_read_response_t *maybe_point_read_response = boost::get<point_read_response_t>(&response.response)) {
            ASSERT_TRUE(maybe_point_read_response->data.has());
            ASSERT_EQ(ql::datum_t(ql::datum_t::construct_null_t()),
                      *maybe_point_read_response->data);
        } else {
            ADD_FAILURE() << "got wrong result back";
        }
    }
}

TEST(RDBProtocol, GetSet) {
    run_in_thread_pool_with_namespace_interface(&run_get_set_test, false);
}

TEST(RDBProtocol, OvershardedGetSet) {
    run_in_thread_pool_with_namespace_interface(&run_get_set_test, true);
}

std::string create_sindex(namespace_interface_t *nsi,
                          order_source_t *osource) {
    std::string id = uuid_to_str(generate_uuid());

    const ql::sym_t arg(1);
    ql::protob_t<const Term> mapping = ql::r::var(arg)["sid"].release_counted();

    ql::map_wire_func_t m(mapping, make_vector(arg), get_backtrace(mapping));

    write_t write(sindex_create_t(id, m, sindex_multi_bool_t::SINGLE),
                  profile_bool_t::PROFILE, ql::configured_limits_t());
    write_response_t response;

    cond_t interruptor;
    nsi->write(write, &response, osource->check_in("unittest::create_sindex(rdb_protocol.cc-A"), &interruptor);

    if (!boost::get<sindex_create_response_t>(&response.response)) {
        ADD_FAILURE() << "got wrong type of result back";
    }

    return id;
}

void wait_for_sindex(namespace_interface_t *nsi,
                          order_source_t *osource,
                          const std::string &id) {
    std::set<std::string> sindexes;
    sindexes.insert(id);
    for (int attempts = 0; attempts < 35; ++attempts) {
        sindex_status_t d(sindexes);
        read_t read(d, profile_bool_t::PROFILE);
        read_response_t response;

        cond_t interruptor;
        nsi->read(read, &response, osource->check_in("unittest::wait_for_sindex(rdb_protocol_t.cc-A"), &interruptor);

        sindex_status_response_t *res =
            boost::get<sindex_status_response_t>(&response.response);

        if (res == NULL) {
            ADD_FAILURE() << "got wrong type of result back";
        }

        auto it = res->statuses.find(id);
        if (it != res->statuses.end() && it->second.ready) {
            return;
        } else {
            nap((attempts+1) * 50);
        }
    }
    ADD_FAILURE() << "Waiting for sindex " << id << " timed out.";
}

bool drop_sindex(namespace_interface_t *nsi,
                 order_source_t *osource,
                 const std::string &id) {
    sindex_drop_t d(id);
    write_t write(d, profile_bool_t::PROFILE, ql::configured_limits_t());
    write_response_t response;

    cond_t interruptor;
    nsi->write(write, &response, osource->check_in("unittest::drop_sindex(rdb_protocol.cc-A"), &interruptor);

    sindex_drop_response_t *res = boost::get<sindex_drop_response_t>(&response.response);

    if (res == NULL) {
        ADD_FAILURE() << "got wrong type of result back";
    }

    return res->success;
}

void run_create_drop_sindex_test(namespace_interface_t *nsi, order_source_t *osource) {
    /* Create a secondary index. */
    std::string id = create_sindex(nsi, osource);
    wait_for_sindex(nsi, osource, id);

    std::shared_ptr<const scoped_cJSON_t> data(
        new scoped_cJSON_t(cJSON_Parse("{\"id\" : 0, \"sid\" : 1}")));
    ql::configured_limits_t limits;
    counted_t<const ql::datum_t> d
        = ql::to_datum(cJSON_slow_GetObjectItem(data->get(), "id"), limits);
    store_key_t pk = store_key_t(d->print_primary());
    counted_t<const ql::datum_t> sindex_key_literal = make_counted<ql::datum_t>(1.0);

    ASSERT_TRUE(data->get());
    {
        /* Insert a piece of data (it will be indexed using the secondary
         * index). */
        write_t write(
            point_write_t(pk, ql::to_datum(data->get(), limits)),
            DURABILITY_REQUIREMENT_DEFAULT,
            profile_bool_t::PROFILE,
            ql::configured_limits_t());
        write_response_t response;

        cond_t interruptor;
        nsi->write(write,
                   &response,
                   osource->check_in(
                       "unittest::run_create_drop_sindex_test(rdb_protocol.cc-A"),
                   &interruptor);

        if (point_write_response_t *maybe_point_write_response
            = boost::get<point_write_response_t>(&response.response)) {
            ASSERT_EQ(maybe_point_write_response->result, point_write_result_t::STORED);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        /* Access the data using the secondary index. */
        read_t read = make_sindex_read(sindex_key_literal, id);
        read_response_t response;

        cond_t interruptor;
        nsi->read(read, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol.cc-A"), &interruptor);

        if (rget_read_response_t *rget_resp = boost::get<rget_read_response_t>(&response.response)) {
            auto streams = boost::get<ql::grouped_t<ql::stream_t> >(
                &rget_resp->result);
            ASSERT_TRUE(streams != NULL);
            ASSERT_EQ(1, streams->size());
            // Order doesn't matter because streams->size() is 1.
            auto stream = &streams->begin(ql::grouped::order_doesnt_matter_t())->second;
            ASSERT_TRUE(stream != NULL);
            ASSERT_EQ(1u, stream->size());
            ASSERT_EQ(*ql::to_datum(data->get(), limits), *stream->at(0).data);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        /* Delete the data. */
        point_delete_t del(pk);
        write_t write(del, DURABILITY_REQUIREMENT_DEFAULT, profile_bool_t::PROFILE,
                      ql::configured_limits_t());
        write_response_t response;

        cond_t interruptor;
        nsi->write(write, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol.cc-A"), &interruptor);

        if (point_delete_response_t *maybe_point_delete_response = boost::get<point_delete_response_t>(&response.response)) {
            ASSERT_EQ(maybe_point_delete_response->result, point_delete_result_t::DELETED);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        /* Access the data using the secondary index. */
        read_t read = make_sindex_read(sindex_key_literal, id);
        read_response_t response;

        cond_t interruptor;
        nsi->read(read, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol.cc-A"), &interruptor);

        if (rget_read_response_t *rget_resp = boost::get<rget_read_response_t>(&response.response)) {
            auto streams = boost::get<ql::grouped_t<ql::stream_t> >(
                &rget_resp->result);
            ASSERT_TRUE(streams != NULL);
            ASSERT_EQ(0, streams->size());
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        const bool drop_sindex_res = drop_sindex(nsi, osource, id);
        ASSERT_TRUE(drop_sindex_res);
    }
}

void populate_sindex(namespace_interface_t *nsi,
                     order_source_t *osource,
                     int num_docs) {
    for (int i = 0; i < num_docs; ++i) {
        std::string json_doc = strprintf("{\"id\" : %d, \"sid\" : %d}", i, i % 4);
        std::shared_ptr<const scoped_cJSON_t> data(
            new scoped_cJSON_t(cJSON_Parse(json_doc.c_str())));
        ql::configured_limits_t limits;
        counted_t<const ql::datum_t> d
            = ql::to_datum(cJSON_slow_GetObjectItem(data->get(), "id"), limits);
        store_key_t pk = store_key_t(d->print_primary());

        /* Insert a piece of data (it will be indexed using the secondary
         * index). */
        write_t write(point_write_t(pk, ql::to_datum(data->get(), limits)),
                      DURABILITY_REQUIREMENT_SOFT, profile_bool_t::PROFILE, limits);
        write_response_t response;

        cond_t interruptor;
        nsi->write(write,
                   &response,
                   osource->check_in(
                       "unittest::run_create_drop_sindex_with_data_test(rdb_protocol.cc-A"),
                   &interruptor);

        /* The result can be either STORED or DUPLICATE (in case this
         * test has been run before on the same store). Either is fine.*/
        if (!boost::get<point_write_response_t>(&response.response)) {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }
}

void run_create_drop_sindex_with_data_test(namespace_interface_t *nsi,
                                           order_source_t *osource,
                                           int num_docs) {
    /* Create a secondary index. */
    std::string id = create_sindex(nsi, osource);
    wait_for_sindex(nsi, osource, id);

    populate_sindex(nsi, osource, num_docs);

    {
        const bool drop_sindex_res = drop_sindex(nsi, osource, id);
        ASSERT_TRUE(drop_sindex_res);
    }
}

void run_repeated_sindex_test(namespace_interface_t *nsi,
                              order_source_t *osource,
                              void (*fn)(namespace_interface_t *, order_source_t *, int)) {
    // Run the test with just a few documents in it
    fn(nsi, osource, 128);
    // ... and just for fun sometimes do it a second time immediately after.
    if (randint(4) == 0) {
        fn(nsi, osource, 128);
    }

    // Nap for a random time before we shut down the namespace interface
    // (in 3 out of 4 cases).
    if (randint(4) != 0) {
        nap(randint(200));
    }
}

TEST(RDBProtocol, SindexCreateDrop) {
    run_in_thread_pool_with_namespace_interface(&run_create_drop_sindex_test, false);
}

TEST(RDBProtocol, SindexRepeatedCreateDrop) {
    // Repeat the test 10 times on the same data files.
    run_in_thread_pool_with_namespace_interface(
        std::bind(&run_repeated_sindex_test,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  &run_create_drop_sindex_with_data_test),
        false,
        10);
}

TEST(RDBProtocol, OvershardedSindexCreateDrop) {
    run_in_thread_pool_with_namespace_interface(&run_create_drop_sindex_test, true);
}

void rename_sindex(namespace_interface_t *nsi,
                   order_source_t *osource,
                   std::string old_name,
                   std::string new_name,
                   bool overwrite,
                   sindex_rename_result_t expected_result) {
    cond_t dummy_interruptor;
    write_t write(sindex_rename_t(old_name, new_name, overwrite),
                  profile_bool_t::PROFILE, ql::configured_limits_t());
    write_response_t response;
    nsi->write(write, &response,
               osource->check_in("unittest::rename_sindex(rdb_protocol.cc-A"),
               &dummy_interruptor);

    sindex_rename_response_t *res = boost::get<sindex_rename_response_t>(&response.response);
    if (res == NULL) {
        ADD_FAILURE() << "got wrong type of result back";
    }
    ASSERT_EQ(expected_result, res->result);
}

void read_sindex(namespace_interface_t *nsi,
                 order_source_t *osource,
                 double value,
                 std::string index,
                 size_t expected_size) {
    /* Access the data using the secondary index. */
    counted_t<const ql::datum_t> key_literal = make_counted<ql::datum_t>(value);
    read_t read = make_sindex_read(key_literal, index);
    read_response_t response;

    cond_t interruptor;
    nsi->read(read, &response, osource->check_in("unittest::run_rename_sindex_test(rdb_protocol.cc-A"), &interruptor);

    if (rget_read_response_t *rget_resp = boost::get<rget_read_response_t>(&response.response)) {
        auto streams = boost::get<ql::grouped_t<ql::stream_t> >(
            &rget_resp->result);
        ASSERT_TRUE(streams != NULL);
        ASSERT_EQ(1, streams->size());
        // Order doesn't matter because streams->size() is 1.
        ql::stream_t *stream
            = &streams->begin(ql::grouped::order_doesnt_matter_t())->second;
        ASSERT_TRUE(stream != NULL);
        ASSERT_EQ(expected_size, stream->size());
    } else {
        ADD_FAILURE() << "got wrong type of result back";
    }
}

void run_rename_sindex_test(namespace_interface_t *nsi,
                            order_source_t *osource,
                            int num_rows) {
    bool sindex_before_data = randint(2) == 0;
    std::string id1;
    std::string id2;

    if (sindex_before_data) {
        id1 = create_sindex(nsi, osource);
        id2 = create_sindex(nsi, osource);
    }

    populate_sindex(nsi, osource, num_rows);

    if (!sindex_before_data) {
        id1 = create_sindex(nsi, osource);
        id2 = create_sindex(nsi, osource);
    }

    wait_for_sindex(nsi, osource, id1);
    wait_for_sindex(nsi, osource, id2);

    // Do a bunch of renaming with and without errors
    rename_sindex(nsi, osource,
                  id1, id2, false,
                  sindex_rename_result_t::NEW_NAME_EXISTS);

    rename_sindex(nsi, osource,
                  "fake", id2, false,
                  sindex_rename_result_t::NEW_NAME_EXISTS);

    rename_sindex(nsi, osource,
                  id1, id2, true,
                  sindex_rename_result_t::SUCCESS);

    rename_sindex(nsi, osource,
                  id1, id2, true,
                  sindex_rename_result_t::OLD_NAME_DOESNT_EXIST);

    rename_sindex(nsi, osource,
                  id2, "fake", true,
                  sindex_rename_result_t::SUCCESS);

    rename_sindex(nsi, osource,
                  "fake", "last", true,
                  sindex_rename_result_t::SUCCESS);

    rename_sindex(nsi, osource,
                  "fake", "fake2", false,
                  sindex_rename_result_t::OLD_NAME_DOESNT_EXIST);

    // At this point the only sindex should be 'last'
    // Perform a read to make sure it survived all the moves
    read_sindex(nsi, osource, 3.0, "last", num_rows / 4);

    {
        const bool drop_sindex_res = drop_sindex(nsi, osource, "last");
        ASSERT_TRUE(drop_sindex_res);
    }
}

TEST(RDBProtocol, SindexRename) {
    run_in_thread_pool_with_namespace_interface(
        std::bind(&run_rename_sindex_test,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  64),
        false);
}

TEST(RDBProtocol, OvershardedSindexRename) {
    run_in_thread_pool_with_namespace_interface(
        std::bind(&run_rename_sindex_test,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  64),
        true);
}

TEST(RDBProtocol, SindexRepeatedRename) {
    // Repeat the test 10 times on the same data files.
    run_in_thread_pool_with_namespace_interface(
        std::bind(&run_repeated_sindex_test,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  &run_rename_sindex_test),
        false,
        10);
}

std::set<std::string> list_sindexes(namespace_interface_t *nsi, order_source_t *osource) {
    sindex_list_t l;
    read_t read(l, profile_bool_t::PROFILE);
    read_response_t response;

    cond_t interruptor;
    nsi->read(read, &response, osource->check_in("unittest::list_sindexes(rdb_protocol.cc-A"), &interruptor);

    sindex_list_response_t *res = boost::get<sindex_list_response_t>(&response.response);

    if (res == NULL) {
        ADD_FAILURE() << "got wrong type of result back";
    }

    return std::set<std::string>(res->sindexes.begin(), res->sindexes.end());
}

void run_sindex_list_test(namespace_interface_t *nsi, order_source_t *osource) {
    std::set<std::string> sindexes;

    // Do the whole test a couple times on the same namespace for kicks
    for (size_t i = 0; i < 2; ++i) {
        size_t reps = 10;

        // Make a bunch of sindexes
        for (size_t j = 0; j < reps; ++j) {
            ASSERT_TRUE(list_sindexes(nsi, osource) == sindexes);
            sindexes.insert(create_sindex(nsi, osource));
        }

        // Remove all the sindexes
        for (size_t j = 0; j < reps; ++j) {
            ASSERT_TRUE(list_sindexes(nsi, osource) == sindexes);
            ASSERT_TRUE(drop_sindex(nsi, osource, *sindexes.begin()));
            sindexes.erase(sindexes.begin());
        }
        ASSERT_TRUE(sindexes.empty());
        ASSERT_TRUE(list_sindexes(nsi, osource).empty());
    } // Do it again
}

TEST(RDBProtocol, SindexList) {
    run_in_thread_pool_with_namespace_interface(&run_sindex_list_test, false);
}

TEST(RDBProtocol, OvershardedSindexList) {
    run_in_thread_pool_with_namespace_interface(&run_sindex_list_test, true);
}

void run_sindex_oversized_keys_test(namespace_interface_t *nsi, order_source_t *osource) {
    std::string sindex_id = create_sindex(nsi, osource);
    wait_for_sindex(nsi, osource, sindex_id);
    ql::configured_limits_t limits;

    for (size_t i = 0; i < 20; ++i) {
        for (size_t j = 100; j < 200; j += 5) {
            std::string id(i + rdb_protocol::MAX_PRIMARY_KEY_SIZE - 10,
                           static_cast<char>(j));
            std::string sid(j, 'a');
            auto sindex_key_literal = make_counted<const ql::datum_t>(std::string(sid));
            std::shared_ptr<const scoped_cJSON_t> data(
                new scoped_cJSON_t(cJSON_CreateObject()));
            cJSON_AddItemToObject(data->get(), "id", cJSON_CreateString(id.c_str()));
            cJSON_AddItemToObject(data->get(), "sid", cJSON_CreateString(sid.c_str()));
            store_key_t pk;
            try {
                pk = store_key_t(ql::to_datum(
                                     cJSON_slow_GetObjectItem(data->get(), "id"),
                                     limits)->print_primary());
            } catch (const ql::base_exc_t &ex) {
                ASSERT_TRUE(id.length() >= rdb_protocol::MAX_PRIMARY_KEY_SIZE);
                continue;
            }
            ASSERT_TRUE(data->get());

            {
                /* Insert a piece of data (it will be indexed using the secondary
                 * index). */
                write_t write(point_write_t(pk, ql::to_datum(data->get(), limits)),
                              DURABILITY_REQUIREMENT_DEFAULT,
                              profile_bool_t::PROFILE,
                              limits);
                write_response_t response;

                cond_t interruptor;
                nsi->write(write,
                           &response,
                           osource->check_in(
                               "unittest::run_sindex_oversized_keys_test("
                               "rdb_protocol.cc-A"),
                           &interruptor);

                auto resp = boost::get<point_write_response_t>(
                        &response.response);
                if (!resp) {
                    ADD_FAILURE() << "got wrong type of result back";
                } else {
                    ASSERT_EQ(resp->result, point_write_result_t::STORED);
                }
            }

            {
                /* Access the data using the secondary index. */
                read_t read
                    = make_sindex_read(sindex_key_literal, sindex_id);
                read_response_t response;

                cond_t interruptor;
                nsi->read(read, &response, osource->check_in("unittest::run_sindex_oversized_keys_test(rdb_protocol.cc-A"), &interruptor);

                if (rget_read_response_t *rget_resp = boost::get<rget_read_response_t>(&response.response)) {
                    auto streams = boost::get<ql::grouped_t<ql::stream_t> >(
                        &rget_resp->result);
                    ASSERT_TRUE(streams != NULL);
                    ASSERT_EQ(1, streams->size());
                    // Order doesn't matter because streams->size() is 1.
                    auto stream = &streams->begin(ql::grouped::order_doesnt_matter_t())->second;
                    ASSERT_TRUE(stream != NULL);
                    // There should be results equal to the number of iterations
                    // performed
                    ASSERT_EQ(i + 1, stream->size());
                } else {
                    ADD_FAILURE() << "got wrong type of result back";
                }
            }
        }
    }

}

TEST(RDBProtocol, OverSizedKeys) {
    run_in_thread_pool_with_namespace_interface(&run_sindex_oversized_keys_test, false);
}

TEST(RDBProtocol, OvershardedOverSizedKeys) {
    run_in_thread_pool_with_namespace_interface(&run_sindex_oversized_keys_test, true);
}

void run_sindex_missing_attr_test(namespace_interface_t *nsi, order_source_t *osource) {
    create_sindex(nsi, osource);

    ql::configured_limits_t limits;
    std::shared_ptr<const scoped_cJSON_t> data(
        new scoped_cJSON_t(cJSON_Parse("{\"id\" : 0}")));
    store_key_t pk = store_key_t(ql::to_datum(
                                     cJSON_slow_GetObjectItem(data->get(), "id"),
                                     limits)->print_primary());
    ASSERT_TRUE(data->get());
    {
        /* Insert a piece of data (it will be indexed using the secondary
         * index). */
        write_t write(point_write_t(pk, ql::to_datum(data->get(), limits)),
                      DURABILITY_REQUIREMENT_DEFAULT,
                      profile_bool_t::PROFILE,
                      ql::configured_limits_t());
        write_response_t response;

        cond_t interruptor;
        nsi->write(write,
                   &response,
                   osource->check_in(
                       "unittest::run_sindex_missing_attr_test(rdb_protocol.cc-A"),
                   &interruptor);

        if (!boost::get<point_write_response_t>(&response.response)) {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    //TODO we're not sure if data which is missing an attribute should be put
    //in the sindex or not right now. We should either be checking that the
    //value is in the sindex right now or be checking that it isn't.
}

TEST(RDBProtocol, MissingAttr) {
    run_in_thread_pool_with_namespace_interface(&run_sindex_missing_attr_test, false);
}

TEST(RDBProtocol, OvershardedMissingAttr) {
    run_in_thread_pool_with_namespace_interface(&run_sindex_missing_attr_test, true);
}

}   /* namespace unittest */
