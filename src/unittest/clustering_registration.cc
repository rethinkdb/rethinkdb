// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/generic/registrar.hpp"
#include "clustering/generic/registrant.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

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

}   /* anonymous namespace */

/* `Register` tests registration, updating, and deregistration of a single
registrant. */
TPTEST(ClusteringRegistration, Register) {
    simple_mailbox_cluster_t cluster;

    monitoring_controller_t controller;
    registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t> registrar(
        cluster.get_mailbox_manager(),
        &controller);

    EXPECT_FALSE(controller.has_registrant);

    {
        registrant_t<std::string> registrant(
            cluster.get_mailbox_manager(),
            registrar.get_business_card(),
            "hello");
        let_stuff_happen();

        EXPECT_TRUE(controller.has_registrant);
        EXPECT_EQ("hello", controller.registrant_data);
    }
    let_stuff_happen();

    EXPECT_FALSE(controller.has_registrant);
}

/* `RegistrarDeath` tests the case where the registrar dies while the registrant
is registered. */
TPTEST(ClusteringRegistration, RegistrarDeath) {
    simple_mailbox_cluster_t cluster;

    monitoring_controller_t controller;

    /* Set up `registrar` in a `scoped_ptr_t` so we can destroy it whenever
    we want to */
    scoped_ptr_t<registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t> > registrar(
        new registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t>(
            cluster.get_mailbox_manager(),
            &controller));

    EXPECT_FALSE(controller.has_registrant);

    registrant_t<std::string> registrant(
        cluster.get_mailbox_manager(),
        registrar->get_business_card(),
        "hello");
    let_stuff_happen();

    EXPECT_TRUE(controller.has_registrant);
    EXPECT_EQ("hello", controller.registrant_data);

    /* Kill the registrar */
    registrar.reset();

    let_stuff_happen();

    EXPECT_FALSE(controller.has_registrant);
}

/* `QuickDisconnect` is to expose a bug that could appear if the registrant is
deleted immediately after being created. */
TPTEST(ClusteringRegistration, QuickDisconnect) {
    simple_mailbox_cluster_t cluster;

    monitoring_controller_t controller;
    registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t> registrar(
        cluster.get_mailbox_manager(),
        &controller);

    EXPECT_FALSE(controller.has_registrant);

    {
        registrant_t<std::string> registrant(
            cluster.get_mailbox_manager(),
            registrar.get_business_card(),
            "hello");
    }
    let_stuff_happen();

    EXPECT_FALSE(controller.has_registrant);
}

}   /* namespace unittest */
