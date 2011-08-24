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

}   /* anonymous namespace */

/* `Register` tests registration, updating, and deregistration of a single
registrant. */

class monitoring_controller_t {

public:
    class registrant_t {
    public:
        registrant_t(monitoring_controller_t *c, std::string data) : parent(c) {
            EXPECT_FALSE(parent->has_registrant);
            parent->has_registrant = true;
            parent->registrant_data = data;
        }
        void update(std::string data) {
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

    cond_t registration_interruptor;
    registrant_t<std::string> registrant(
        &cluster,
        metadata_controller.get_view(),
        "hello",
        &registration_interruptor);

    EXPECT_FALSE(registrant.get_failed_signal()->is_pulsed());
    EXPECT_TRUE(controller.has_registrant);
    EXPECT_EQ("hello", controller.registrant_data);

    cond_t update_interruptor;
    registrant.update("updated", &update_interruptor);

    EXPECT_FALSE(registrant.get_failed_signal()->is_pulsed());
    EXPECT_TRUE(controller.has_registrant);
    EXPECT_EQ("updated", controller.registrant_data);

    cond_t deregister_interruptor;
    registrant.deregister(&deregister_interruptor);

    EXPECT_FALSE(controller.has_registrant);
}
TEST(ClusteringRegistration, Register) {
    run_in_thread_pool(&run_register_test);
}

}   /* namespace unittest */
