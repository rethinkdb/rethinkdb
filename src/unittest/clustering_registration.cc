// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/generic/registrar.hpp"
#include "clustering/generic/registrant.hpp"
#include "mock/clustering_utils.hpp"
#include "mock/unittest_utils.hpp"

namespace unittest {

namespace {

class monitoring_controller_t {

public:
    class registrant_t {
    public:
        registrant_t(monitoring_controller_t *c, const std::string& data) : parent(c) {
            EXPECT_FALSE(parent->has_registrant);
            parent->has_registrant = true;
            parent->registrant_data = data;
        }
        ~registrant_t() {
            EXPECT_TRUE(parent->has_registrant);
            parent->has_registrant = false;
        }
        monitoring_controller_t *parent;
    };

    monitoring_controller_t() : has_registrant(false) { }
    bool has_registrant;
    std::string registrant_data;
};

boost::optional<boost::optional<registrar_business_card_t<std::string> > > wrap_in_optional(
        const boost::optional<registrar_business_card_t<std::string> > &inner) {
    return boost::optional<boost::optional<registrar_business_card_t<std::string> > >(inner);
}

}   /* anonymous namespace */

/* `Register` tests registration, updating, and deregistration of a single
registrant. */

void run_register_test() {

    mock::simple_mailbox_cluster_t cluster;

    monitoring_controller_t controller;
    registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t> registrar(
        cluster.get_mailbox_manager(),
        &controller);

    watchable_variable_t<boost::optional<registrar_business_card_t<std::string> > > metadata_controller(
        boost::optional<registrar_business_card_t<std::string> >(registrar.get_business_card()));

    EXPECT_FALSE(controller.has_registrant);

    {
        registrant_t<std::string> registrant(
            cluster.get_mailbox_manager(),
            metadata_controller.get_watchable()->subview(&wrap_in_optional),
            "hello");
        mock::let_stuff_happen();

        EXPECT_FALSE(registrant.get_failed_signal()->is_pulsed());
        EXPECT_TRUE(controller.has_registrant);
        EXPECT_EQ("hello", controller.registrant_data);
    }
    mock::let_stuff_happen();

    EXPECT_FALSE(controller.has_registrant);
}
TEST(ClusteringRegistration, Register) {
    mock::run_in_thread_pool(&run_register_test);
}

/* `RegistrarDeath` tests the case where the registrar dies while the registrant
is registered. */

void run_registrar_death_test() {

    mock::simple_mailbox_cluster_t cluster;

    monitoring_controller_t controller;

    /* Set up `registrar` in a `scoped_ptr_t` so we can destroy it whenever
    we want to */
    scoped_ptr_t<registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t> > registrar(
        new registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t>(
            cluster.get_mailbox_manager(),
            &controller));

    EXPECT_FALSE(controller.has_registrant);

    watchable_variable_t<boost::optional<registrar_business_card_t<std::string> > > metadata_controller(
        boost::optional<registrar_business_card_t<std::string> >(registrar->get_business_card()));

    registrant_t<std::string> registrant(
        cluster.get_mailbox_manager(),
        metadata_controller.get_watchable()->subview(&wrap_in_optional),
        "hello");
    mock::let_stuff_happen();

    EXPECT_FALSE(registrant.get_failed_signal()->is_pulsed());
    EXPECT_TRUE(controller.has_registrant);
    EXPECT_EQ("hello", controller.registrant_data);

    /* Kill the registrar */
    metadata_controller.set_value(boost::optional<registrar_business_card_t<std::string> >());
    registrar.reset();

    mock::let_stuff_happen();

    EXPECT_TRUE(registrant.get_failed_signal()->is_pulsed());
    EXPECT_FALSE(controller.has_registrant);
}
TEST(ClusteringRegistration, RegistrarDeath) {
    mock::run_in_thread_pool(&run_registrar_death_test);
}

/* `QuickDisconnect` is to expose a bug that could appear if the registrant is
deleted immediately after being created. */

void run_quick_disconnect_test() {

    mock::simple_mailbox_cluster_t cluster;

    monitoring_controller_t controller;
    registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t> registrar(
        cluster.get_mailbox_manager(),
        &controller);

    watchable_variable_t<boost::optional<registrar_business_card_t<std::string> > > metadata_controller(
        boost::optional<registrar_business_card_t<std::string> >(registrar.get_business_card()));

    EXPECT_FALSE(controller.has_registrant);

    {
        registrant_t<std::string> registrant(
            cluster.get_mailbox_manager(),
            metadata_controller.get_watchable()->subview(&wrap_in_optional),
            "hello");
    }
    mock::let_stuff_happen();

    EXPECT_FALSE(controller.has_registrant);
}
TEST(ClusteringRegistration, QuickDisconnect) {
    mock::run_in_thread_pool(&run_quick_disconnect_test);
}

}   /* namespace unittest */
