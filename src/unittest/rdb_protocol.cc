// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "buffer_cache/buffer_cache.hpp"
#include "clustering/administration/metadata.hpp"
#include "containers/iterators.hpp"
#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_spawner.hpp"
#include "memcached/protocol.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rpc/semilattice/semilattice_manager.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "unittest/dummy_namespace_interface.hpp"
#include "unittest/gtest.hpp"

#include "memcached/protocol_json_adapter.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace unittest {
namespace {

void run_with_namespace_interface(boost::function<void(namespace_interface_t<rdb_protocol_t> *, order_source_t *)> fun, bool oversharding) {
    recreate_temporary_directory(base_path_t("."));

    order_source_t order_source;

    /* Pick shards */
    std::vector<rdb_protocol_t::region_t> store_shards;
    if (oversharding) {
        store_shards.push_back(rdb_protocol_t::region_t(key_range_t(key_range_t::none,   store_key_t(),  key_range_t::none, store_key_t())));
    } else {
        store_shards.push_back(rdb_protocol_t::region_t(key_range_t(key_range_t::none,   store_key_t(),  key_range_t::open, store_key_t("n"))));
        store_shards.push_back(rdb_protocol_t::region_t(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t() )));
    }

    std::vector<rdb_protocol_t::region_t> nsi_shards;

    nsi_shards.push_back(rdb_protocol_t::region_t(key_range_t(key_range_t::none,   store_key_t(),  key_range_t::open, store_key_t("n"))));
    nsi_shards.push_back(rdb_protocol_t::region_t(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t() )));

    boost::ptr_vector<temp_file_t> temp_files;
    for (size_t i = 0; i < store_shards.size(); ++i) {
        temp_files.push_back(new temp_file_t);
    }

    io_backender_t io_backender(file_direct_io_mode_t::buffered_desired);

    scoped_array_t<scoped_ptr_t<serializer_t> > serializers(store_shards.size());
    for (size_t i = 0; i < store_shards.size(); ++i) {
        filepath_file_opener_t file_opener(temp_files[i].name(), &io_backender);
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
    extproc_pool_t extproc_pool(2);

    connectivity_cluster_t c;
    semilattice_manager_t<cluster_semilattice_metadata_t> slm(&c, cluster_semilattice_metadata_t());
    connectivity_cluster_t::run_t cr(&c, get_unittest_addresses(), peer_address_t(), ANY_PORT, &slm, 0, NULL);

    connectivity_cluster_t c2;
    directory_read_manager_t<cluster_directory_metadata_t> read_manager(&c2);
    connectivity_cluster_t::run_t cr2(&c2, get_unittest_addresses(), peer_address_t(), ANY_PORT, &read_manager, 0, NULL);

    boost::shared_ptr<semilattice_readwrite_view_t<auth_semilattice_metadata_t> > dummy_auth;
    rdb_protocol_t::context_t ctx(&extproc_pool, NULL, slm.get_root_view(),
                                  dummy_auth, &read_manager, generate_uuid());

    for (size_t i = 0; i < store_shards.size(); ++i) {
        underlying_stores.push_back(
                new rdb_protocol_t::store_t(serializers[i].get(),
                    temp_files[i].name().permanent_path(), GIGABYTE, true,
                    &get_global_perfmon_collection(), &ctx,
                    &io_backender, base_path_t(".")));
    }

    boost::ptr_vector<store_view_t<rdb_protocol_t> > stores;
    for (size_t i = 0; i < nsi_shards.size(); ++i) {
        if (oversharding) {
            stores.push_back(new store_subview_t<rdb_protocol_t>(&underlying_stores[0], nsi_shards[i]));
        } else {
            stores.push_back(new store_subview_t<rdb_protocol_t>(&underlying_stores[i], nsi_shards[i]));
        }
    }

    /* Set up namespace interface */
    dummy_namespace_interface_t<rdb_protocol_t> nsi(nsi_shards, stores.c_array(), &order_source, &ctx);

    fun(&nsi, &order_source);
}

void run_in_thread_pool_with_namespace_interface(boost::function<void(namespace_interface_t<rdb_protocol_t> *, order_source_t*)> fun, bool oversharded) {
    extproc_spawner_t extproc_spawner;
    unittest::run_in_thread_pool(boost::bind(&run_with_namespace_interface, fun, oversharded));
}

}   /* anonymous namespace */

/* `SetupTeardown` makes sure that it can start and stop without anything going
horribly wrong */
void run_setup_teardown_test(UNUSED namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *) {
    /* Do nothing */
}
TEST(RDBProtocol, SetupTeardown) {
    run_in_thread_pool_with_namespace_interface(&run_setup_teardown_test, false);
}

TEST(RDBProtocol, OvershardedSetupTeardown) {
    run_in_thread_pool_with_namespace_interface(&run_setup_teardown_test, true);
}

/* `GetSet` tests basic get and set operations */
void run_get_set_test(namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *osource) {
    {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t("a"),
                                                                    make_counted<ql::datum_t>(ql::datum_t::R_NULL)),
                                      DURABILITY_REQUIREMENT_DEFAULT);
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
            ASSERT_TRUE(maybe_point_read_response->data.has());
            ASSERT_EQ(ql::datum_t(ql::datum_t::R_NULL), *maybe_point_read_response->data);
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

std::string create_sindex(namespace_interface_t<rdb_protocol_t> *nsi,
                          order_source_t *osource) {
    std::string id = uuid_to_str(generate_uuid());

    ql::protob_t<Term> twrap = ql::make_counted_term();
    Term *arg = twrap.get();
    const ql::sym_t one(1);
    N2(GET_FIELD, NVAR(one), NDATUM("sid"));

    ql::map_wire_func_t m(twrap, make_vector(one), get_backtrace(twrap));

    rdb_protocol_t::write_t write(rdb_protocol_t::sindex_create_t(id, m, SINGLE));
    rdb_protocol_t::write_response_t response;

    cond_t interruptor;
    nsi->write(write, &response, osource->check_in("unittest::create_sindex(rdb_protocol_t.cc-A"), &interruptor);

    if (!boost::get<rdb_protocol_t::sindex_create_response_t>(&response.response)) {
        ADD_FAILURE() << "got wrong type of result back";
    }

    return id;
}

bool drop_sindex(namespace_interface_t<rdb_protocol_t> *nsi,
                 order_source_t *osource,
                 const std::string &id) {
    rdb_protocol_t::sindex_drop_t d(id);
    rdb_protocol_t::write_t write(d);
    rdb_protocol_t::write_response_t response;

    cond_t interruptor;
    nsi->write(write, &response, osource->check_in("unittest::drop_sindex(rdb_protocol_t.cc-A"), &interruptor);

    rdb_protocol_t::sindex_drop_response_t *res =
        boost::get<rdb_protocol_t::sindex_drop_response_t>(&response.response);

    if (res == NULL) {
        ADD_FAILURE() << "got wrong type of result back";
    }

    return res->success;
}

void run_create_drop_sindex_test(namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *osource) {
    /* Create a secondary index. */
    std::string id = create_sindex(nsi, osource);

    std::shared_ptr<const scoped_cJSON_t> data(
        new scoped_cJSON_t(cJSON_Parse("{\"id\" : 0, \"sid\" : 1}")));
    counted_t<const ql::datum_t> d(
        new ql::datum_t(cJSON_GetObjectItem(data->get(), "id")));
    store_key_t pk = store_key_t(d->print_primary());
    counted_t<const ql::datum_t> sindex_key_literal = make_counted<ql::datum_t>(1.0);

    ASSERT_TRUE(data->get());
    {
        /* Insert a piece of data (it will be indexed using the secondary
         * index). */
        rdb_protocol_t::write_t write(
            rdb_protocol_t::point_write_t(pk, make_counted<ql::datum_t>(*data)),
            DURABILITY_REQUIREMENT_DEFAULT);
        rdb_protocol_t::write_response_t response;

        cond_t interruptor;
        nsi->write(write,
                   &response,
                   osource->check_in(
                       "unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"),
                   &interruptor);

        if (rdb_protocol_t::point_write_response_t *maybe_point_write_response
            = boost::get<rdb_protocol_t::point_write_response_t>(&response.response)) {
            ASSERT_EQ(maybe_point_write_response->result, STORED);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        /* Access the data using the secondary index. */
        rdb_protocol_t::read_t read(rdb_protocol_t::rget_read_t(
            id, sindex_range_t(sindex_key_literal, false, sindex_key_literal, false)));
        rdb_protocol_t::read_response_t response;

        cond_t interruptor;
        nsi->read(read, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"), &interruptor);

        if (rdb_protocol_t::rget_read_response_t *rget_resp = boost::get<rdb_protocol_t::rget_read_response_t>(&response.response)) {
            rdb_protocol_t::rget_read_response_t::stream_t *stream = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&rget_resp->result);
            ASSERT_TRUE(stream != NULL);
            ASSERT_EQ(1u, stream->size());
            ASSERT_EQ(ql::datum_t(*data), *stream->at(0).data);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        /* Delete the data. */
        rdb_protocol_t::point_delete_t d(pk);
        rdb_protocol_t::write_t write(d, DURABILITY_REQUIREMENT_DEFAULT);
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
        rdb_protocol_t::read_t read(rdb_protocol_t::rget_read_t(
            id, sindex_range_t(sindex_key_literal, false, sindex_key_literal, false)));

        rdb_protocol_t::read_response_t response;

        cond_t interruptor;
        nsi->read(read, &response, osource->check_in("unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"), &interruptor);

        if (rdb_protocol_t::rget_read_response_t *rget_resp = boost::get<rdb_protocol_t::rget_read_response_t>(&response.response)) {
            rdb_protocol_t::rget_read_response_t::stream_t *stream = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&rget_resp->result);
            ASSERT_TRUE(stream != NULL);
            ASSERT_EQ(0u, stream->size());
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    ASSERT_TRUE(drop_sindex(nsi, osource, id));
}

TEST(RDBProtocol, SindexCreateDrop) {
    run_in_thread_pool_with_namespace_interface(&run_create_drop_sindex_test, false);
}

TEST(RDBProtocol, OvershardedSindexCreateDrop) {
    run_in_thread_pool_with_namespace_interface(&run_create_drop_sindex_test, true);
}

std::set<std::string> list_sindexes(namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *osource) {
    rdb_protocol_t::sindex_list_t l;
    rdb_protocol_t::read_t read(l);
    rdb_protocol_t::read_response_t response;

    cond_t interruptor;
    nsi->read(read, &response, osource->check_in("unittest::list_sindexes(rdb_protocol_t.cc-A"), &interruptor);

    rdb_protocol_t::sindex_list_response_t *res = boost::get<rdb_protocol_t::sindex_list_response_t>(&response.response);

    if (res == NULL) {
        ADD_FAILURE() << "got wrong type of result back";
    }

    return std::set<std::string>(res->sindexes.begin(), res->sindexes.end());
}

void run_sindex_list_test(namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *osource) {
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

void run_sindex_oversized_keys_test(namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *osource) {
    std::string sindex_id = create_sindex(nsi, osource);

    for (size_t i = 0; i < 20; ++i) {
        for (size_t j = 100; j < 200; j += 5) {
            std::string id(i + rdb_protocol_t::MAX_PRIMARY_KEY_SIZE - 10,
                           static_cast<char>(j));
            std::string sid(j, 'a');
            auto sindex_key_literal = make_counted<const ql::datum_t>(std::string(sid));
            std::shared_ptr<const scoped_cJSON_t> data(
                new scoped_cJSON_t(cJSON_CreateObject()));
            cJSON_AddItemToObject(data->get(), "id", cJSON_CreateString(id.c_str()));
            cJSON_AddItemToObject(data->get(), "sid", cJSON_CreateString(sid.c_str()));
            store_key_t pk;
            try {
                pk = store_key_t(make_counted<const ql::datum_t>(
                    cJSON_GetObjectItem(data->get(), "id"))->print_primary());
            } catch (const ql::base_exc_t &ex) {
                ASSERT_TRUE(id.length() >= rdb_protocol_t::MAX_PRIMARY_KEY_SIZE);
                continue;
            }
            ASSERT_TRUE(data->get());

            {
                /* Insert a piece of data (it will be indexed using the secondary
                 * index). */
                rdb_protocol_t::write_t write(
                    rdb_protocol_t::point_write_t(pk, make_counted<ql::datum_t>(*data)),
                    DURABILITY_REQUIREMENT_DEFAULT);
                rdb_protocol_t::write_response_t response;

                cond_t interruptor;
                nsi->write(write,
                           &response,
                           osource->check_in(
                               "unittest::run_sindex_oversized_keys_test("
                               "rdb_protocol_t.cc-A"),
                           &interruptor);

                auto resp = boost::get<rdb_protocol_t::point_write_response_t>(
                        &response.response);
                if (!resp) {
                    ADD_FAILURE() << "got wrong type of result back";
                } else {
                    ASSERT_EQ(resp->result, STORED);
                }
            }

            {
                /* Access the data using the secondary index. */
                rdb_protocol_t::rget_read_t rget(
                    sindex_id, sindex_range_t(sindex_key_literal, false, sindex_key_literal, false));
                rdb_protocol_t::read_t read(rget);
                rdb_protocol_t::read_response_t response;

                cond_t interruptor;
                nsi->read(read, &response, osource->check_in("unittest::run_sindex_oversized_keys_test(rdb_protocol_t.cc-A"), &interruptor);

                if (rdb_protocol_t::rget_read_response_t *rget_resp = boost::get<rdb_protocol_t::rget_read_response_t>(&response.response)) {
                    rdb_protocol_t::rget_read_response_t::stream_t *stream = boost::get<rdb_protocol_t::rget_read_response_t::stream_t>(&rget_resp->result);
                    ASSERT_TRUE(stream != NULL);
                    // There should be results equal to the number of iterations performed
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

void run_sindex_missing_attr_test(namespace_interface_t<rdb_protocol_t> *nsi, order_source_t *osource) {
    create_sindex(nsi, osource);

    std::shared_ptr<const scoped_cJSON_t> data(
        new scoped_cJSON_t(cJSON_Parse("{\"id\" : 0}")));
    store_key_t pk = store_key_t(make_counted<const ql::datum_t>(
        cJSON_GetObjectItem(data->get(), "id"))->print_primary());
    ASSERT_TRUE(data->get());
    {
        /* Insert a piece of data (it will be indexed using the secondary
         * index). */
        rdb_protocol_t::write_t write(
            rdb_protocol_t::point_write_t(pk, make_counted<ql::datum_t>(*data)),
            DURABILITY_REQUIREMENT_DEFAULT);
        rdb_protocol_t::write_response_t response;

        cond_t interruptor;
        nsi->write(write,
                   &response,
                   osource->check_in(
                       "unittest::run_create_drop_sindex_test(rdb_protocol_t.cc-A"),
                   &interruptor);

        if (!boost::get<rdb_protocol_t::point_write_response_t>(&response.response)) {
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

