#include "unittest/gtest.hpp"
#include "clustering/registrar.hpp"
#include "clustering/registrant.hpp"
#include "rpc/metadata/view/controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

/* We need a `mailbox_cluster_t` so we can create mailboxes, but we have no use
for its utility messages. `dummy_cluster_t` is a `mailbox_cluster_t` with
stubbed utility message implementation. */

class dummy_cluster_t : public mailbox_cluster_t {
public:
    dummy_cluster_t() : mailbox_cluster_t(10000 + rand() % 20000) { }
private:
    void on_utility_message(peer_id_t, std::istream&, boost::function<void()>&) {
        ADD_FAILURE() << "no utility messages should be sent. WTF?";
    }
};

class monitoring_controller_t {

public:
    class registrant_t {
    public:
        registrant_t(monitoring_controller_t *c, std::string data) : parent(c) {
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

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);
}

}   /* anonymous namespace */

/* `Register` tests registration, updating, and deregistration of a single
registrant. */

void run_register_test() {

    dummy_cluster_t cluster;

    metadata_view_controller_t<resource_metadata_t<registrar_metadata_t<std::string> > > metadata_controller(
        (resource_metadata_t<registrar_metadata_t<std::string> >()));

    monitoring_controller_t controller;
    registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t> registrar(
        &cluster,
        &controller,
        metadata_controller.get_view());

    EXPECT_FALSE(controller.has_registrant);

    {
        registrant_t<std::string> registrant(
            &cluster,
            metadata_controller.get_view(),
            "hello");
        let_stuff_happen();

        EXPECT_FALSE(registrant.get_failed_signal()->is_pulsed());
        EXPECT_TRUE(controller.has_registrant);
        EXPECT_EQ("hello", controller.registrant_data);
    }
    let_stuff_happen();

    EXPECT_FALSE(controller.has_registrant);
}
TEST(ClusteringRegistration, Register) {
    run_in_thread_pool(&run_register_test);
}

/* `RegistrarDeath` tests the case where the registrar dies while the registrant
is registered. */

void run_registrar_death_test() {

    dummy_cluster_t cluster;

    metadata_view_controller_t<resource_metadata_t<registrar_metadata_t<std::string> > > metadata_controller(
        (resource_metadata_t<registrar_metadata_t<std::string> >()));

    monitoring_controller_t controller;

    /* Set up `registrar` in a `boost::scoped_ptr` so we can destroy it whenever
    we want to */
    boost::scoped_ptr<registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t> > registrar(
        new registrar_t<std::string, monitoring_controller_t *, monitoring_controller_t::registrant_t>(
            &cluster,
            &controller,
            metadata_controller.get_view()
        ));

    EXPECT_FALSE(controller.has_registrant);

    registrant_t<std::string> registrant(
        &cluster,
        metadata_controller.get_view(),
        "hello");
    let_stuff_happen();

    EXPECT_FALSE(registrant.get_failed_signal()->is_pulsed());
    EXPECT_TRUE(controller.has_registrant);
    EXPECT_EQ("hello", controller.registrant_data);

    /* Kill the registrar */
    registrar.reset();

    let_stuff_happen();

    EXPECT_TRUE(registrant.get_failed_signal()->is_pulsed());
    EXPECT_FALSE(controller.has_registrant);
}
TEST(ClusteringRegistration, RegistrarDeath) {
    run_in_thread_pool(&run_registrar_death_test);
}

}   /* namespace unittest */
