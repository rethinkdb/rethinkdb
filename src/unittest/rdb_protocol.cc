#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "buffer_cache/buffer_cache.hpp"
#include "containers/iterators.hpp"
#include "rdb_protocol/protocol.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "unittest/gtest.hpp"
#include "unittest/dummy_namespace_interface.hpp"

namespace unittest {
namespace {

void run_with_namespace_interface(boost::function<void(namespace_interface_t<rdb_protocol_t> *)> fun) {
    /* Pick shards */
    std::vector<key_range_t> shards;
    shards.push_back(key_range_t(key_range_t::none,   store_key_t(""),  key_range_t::open, store_key_t("n")));
    shards.push_back(key_range_t(key_range_t::closed, store_key_t("n"), key_range_t::none, store_key_t("") ));

    boost::ptr_vector<mock::temp_file_t> temp_files;
    for (int i = 0; i < (int)shards.size(); i++) {
        temp_files.push_back(new mock::temp_file_t("/tmp/rdb_unittest.XXXXXX"));
    }

    boost::ptr_vector<rdb_protocol_t::store_t> underlying_stores;
    boost::scoped_ptr<io_backender_t> io_backender;
    make_io_backender(aio_default, &io_backender);

    for (int i = 0; i < (int)shards.size(); i++) {
        underlying_stores.push_back(new rdb_protocol_t::store_t(io_backender.get(), temp_files[i].name(), true, &get_global_perfmon_collection()));
    }

    std::vector<boost::shared_ptr<store_view_t<rdb_protocol_t> > > stores;
    for (int i = 0; i < (int)shards.size(); i++) {
        stores.push_back(boost::make_shared<store_subview_t<rdb_protocol_t> >(&underlying_stores[i], shards[i]));
    }

    /* Set up namespace interface */
    dummy_namespace_interface_t<rdb_protocol_t> nsi(shards, stores);

    fun(&nsi);
}

void run_in_thread_pool_with_namespace_interface(boost::function<void(namespace_interface_t<rdb_protocol_t> *)> fun) {
    mock::run_in_thread_pool(boost::bind(&run_with_namespace_interface, fun));
}

}   /* anonymous namespace */

/* `SetupTeardown` makes sure that it can start and stop without anything going
horribly wrong */
void run_setup_teardown_test(UNUSED namespace_interface_t<rdb_protocol_t> *nsi) {
    /* Do nothing */
}
TEST(RDBProtocol, SetupTeardown) {
    run_in_thread_pool_with_namespace_interface(&run_setup_teardown_test);
}

/* `GetSet` tests basic get and set operations */
void run_get_set_test(namespace_interface_t<rdb_protocol_t> *nsi) {
    order_source_t osource;
    boost::shared_ptr<scoped_cJSON_t> data(new scoped_cJSON_t(cJSON_CreateNull()));
    {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t("a"), data));

        cond_t interruptor;
        rdb_protocol_t::write_response_t response = nsi->write(write, osource.check_in("unittest::run_get_set_test(rdb_protocol.cc-A)"), &interruptor);

        if (rdb_protocol_t::point_write_response_t *maybe_point_write_response_t = boost::get<rdb_protocol_t::point_write_response_t>(&response.response)) {
            EXPECT_EQ(maybe_point_write_response_t->result, STORED);
        } else {
            ADD_FAILURE() << "got wrong type of result back";
        }
    }

    {
        rdb_protocol_t::read_t read(rdb_protocol_t::point_read_t(store_key_t("a")));

        cond_t interruptor;
        rdb_protocol_t::read_response_t response = nsi->read(read, osource.check_in("unittest::run_get_set_test(rdb_protocol.cc-B)"), &interruptor);

        if (rdb_protocol_t::point_read_response_t *maybe_point_read_response = boost::get<rdb_protocol_t::point_read_response_t>(&response.response)) {
            EXPECT_TRUE(maybe_point_read_response->data->get() != NULL);
            EXPECT_TRUE(cJSON_Equal(data->get(), maybe_point_read_response->data->get()));
        } else {
            ADD_FAILURE() << "got wrong result back";
        }
    }
}

TEST(RDBProtocol, GetSet) {
    run_in_thread_pool_with_namespace_interface(&run_get_set_test);
}

}   /* namespace unittest */

