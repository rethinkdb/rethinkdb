#include "unittest/dummy_namespace_interface.hpp"
#include "unittest/gtest.hpp"
#include "mock/dummy_protocol.hpp"

/* This file tests `dummy_protocol_t` and `dummy_namespace_interface_t` against
each other. */

namespace unittest {

using mock::dummy_protocol_t;

namespace {

void run_with_namespace_interface(boost::function<void(namespace_interface_t<dummy_protocol_t> *, order_source_t *)> fun) {

    order_source_t order_source;

    std::vector<dummy_protocol_t::region_t> shards;

    dummy_protocol_t::region_t region1;
    for (char c = 'a'; c <= 'm'; c++) region1.keys.insert(std::string(&c, 1));
    shards.push_back(region1);

    dummy_protocol_t::region_t region2;
    for (char c = 'n'; c <= 'z'; c++) region2.keys.insert(std::string(&c, 1));
    shards.push_back(region2);

    boost::ptr_vector<dummy_protocol_t::store_t> underlying_stores;
    boost::ptr_vector<store_view_t<dummy_protocol_t> > stores;
    for (size_t i = 0; i < shards.size(); ++i) {
        underlying_stores.push_back(new dummy_protocol_t::store_t);
        stores.push_back(new store_subview_t<dummy_protocol_t>(&underlying_stores[i], shards[i]));
    }

    dummy_namespace_interface_t<dummy_protocol_t> nsi(shards, stores.c_array(), &order_source);

    fun(&nsi, &order_source);
}

void run_in_thread_pool_with_namespace_interface(boost::function<void(namespace_interface_t<dummy_protocol_t> *, order_source_t *)> fun) {
    run_in_thread_pool(boost::bind(&run_with_namespace_interface, fun));
}

}   /* anonymous namespace */

/* `SetupTeardown` makes sure that it can start and stop without anything going
horribly wrong */

void run_setup_teardown_test(UNUSED namespace_interface_t<dummy_protocol_t> *nsi, UNUSED order_source_t *order_source) {
    /* Do nothing */
}
TEST(DummyProtocolNamespaceInterface, SetupTeardown) {
    run_in_thread_pool_with_namespace_interface(&run_setup_teardown_test);
}

/* `GetSet` tests basic get and set operations */

void run_get_set_test(namespace_interface_t<dummy_protocol_t> *nsi, order_source_t *order_source) {

    {
        dummy_protocol_t::write_t w;
        dummy_protocol_t::write_response_t wr;
        w.values["a"] = "floop";

        cond_t interruptor;
        nsi->write(w, &wr, order_source->check_in("unittest::run_get_set_test(A)"), &interruptor);

        EXPECT_EQ(wr.old_values.size(), 1);
        EXPECT_EQ(wr.old_values["a"], "");
    }

    {
        dummy_protocol_t::write_t w;
        dummy_protocol_t::write_response_t wr;
        w.values["a"] = "flup";
        w.values["q"] = "flarp";

        cond_t interruptor;
        nsi->write(w, &wr, order_source->check_in("unittest::run_get_set_test(B)"), &interruptor);

        EXPECT_EQ(wr.old_values.size(), 2);
        EXPECT_EQ(wr.old_values["a"], "floop");
        EXPECT_EQ(wr.old_values["q"], "");
    }

    {
        dummy_protocol_t::read_t r;
        dummy_protocol_t::read_response_t rr;
        r.keys.keys.insert("a");
        r.keys.keys.insert("q");
        r.keys.keys.insert("z");

        cond_t interruptor;
        nsi->read(r, &rr, order_source->check_in("unittest::run_get_set_test(C)").with_read_mode(), &interruptor);

        EXPECT_EQ(rr.values.size(), 3);
        EXPECT_EQ(rr.values["a"], "flup");
        EXPECT_EQ(rr.values["q"], "flarp");
        EXPECT_EQ(rr.values["z"], "");
    }
}
TEST(DummyProtocolNamespaceInterface, GetSet) {
    run_in_thread_pool_with_namespace_interface(&run_get_set_test);
}

}   /* namespace unittest */
