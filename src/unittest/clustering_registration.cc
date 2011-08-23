#include "unittest/gtest.hpp"
#include "clustering/registrar.hpp"
#include "clustering/registrant.hpp"
#include "rpc/metadata/view/controller.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

namespace {

struct incomplete_dummy_protocol_t {

    typedef std::string read_t;
    typedef std::string write_t;
    typedef std::string read_response_t;
    typedef std::string write_response_t;
};

class dummy_callback_t : public registrar_t<incomplete_dummy_protocol_t>::callback_t {

public:
    dummy_callback_t() : registrant(NULL) { }

    void on_register(registrar_t<incomplete_dummy_protocol_t>::registrant_t *r) {
        EXPECT_TRUE(registrant == NULL);
        EXPECT_FALSE(r == NULL);
        registrant = r;
        upgraded = false;
    }

    void on_upgrade_to_reads(registrar_t<incomplete_dummy_protocol_t>::registrant_t *r) {
        EXPECT_FALSE(registrant == NULL);
        EXPECT_EQ(r, registrant);
        EXPECT_FALSE(upgraded);
        upgraded = true;
    }

    void on_deregister(registrar_t<incomplete_dummy_protocol_t>::registrant_t *r) {
        EXPECT_FALSE(registrant == NULL);
        EXPECT_EQ(r, registrant);
        registrant = NULL;
    }

    registrar_t<incomplete_dummy_protocol_t>::registrant_t *registrant;
    bool upgraded;
};

class dummy_query_handler_t {

public:
    void handle_write(std::string write, repli_timestamp_t, order_token_t tok) {
        osink.check_out(tok);
        log.push_back(write);
    }

    std::string handle_writeread(std::string writeread, repli_timestamp_t, order_token_t tok) {
        osink.check_out(tok);
        log.push_back(writeread);
        return "handle_writeread(" + writeread + ")";
    }

    std::string handle_read(std::string read, order_token_t tok) {
        osink.check_out(tok);
        log.push_back(read);
        return "handle_read(" + read + ")";
    }

    std::vector<std::string> log;
    repli_timestamp_t ts;
    order_sink_t osink;
};

}   /* anonymous namespace */

void run_register_test() {

    /* Lots of stuff needs an interruptor that will never be pulsed. Here it is.
    */
    cond_t interruptor;

    /* Set up a cluster so mailboxes can be created */

    class dummy_cluster_t : public mailbox_cluster_t {
    public:
        dummy_cluster_t(int port) : mailbox_cluster_t(port) { }
    private:
        void on_utility_message(peer_id_t, std::istream&, boost::function<void()>&) {
            ADD_FAILURE() << "no utility messages should be sent. WTF?";
        }
    } cluster(10000 + rand() % 20000);

    /* Set up a registrar */

    dummy_callback_t callback;

    metadata_view_controller_t<resource_metadata_t<registrar_metadata_t<incomplete_dummy_protocol_t> > > registrar_md_controller(
        (resource_metadata_t<registrar_metadata_t<incomplete_dummy_protocol_t> >()));

    registrar_t<incomplete_dummy_protocol_t> registrar(
        &cluster,
        &callback,
        registrar_md_controller.get_view());

    order_source_t osource;

    {
        /* Set up a registrant */

        dummy_query_handler_t handler;

        registrant_t<incomplete_dummy_protocol_t> registrant(
            &cluster,
            registrar_md_controller.get_view(),
            boost::bind(&dummy_query_handler_t::handle_write, &handler, _1, _2, _3),
            &interruptor);

        /* Make sure everything went as expected */

        EXPECT_FALSE(registrant.get_failed_signal()->is_pulsed());

        ASSERT_TRUE(callback.registrant != NULL);
        EXPECT_FALSE(callback.upgraded);

        /* Test a write query */

        callback.registrant->write("w1", repli_timestamp_t::invalid, osource.check_in("unittest"), &interruptor);
        EXPECT_EQ(handler.log.size(), 1);
        EXPECT_EQ(handler.log[0], "w1");

        /* Upgrade to reads */

        registrant.upgrade_to_reads(
            boost::bind(&dummy_query_handler_t::handle_writeread, &handler, _1, _2, _3),
            boost::bind(&dummy_query_handler_t::handle_read, &handler, _1, _2));

        EXPECT_FALSE(registrant.get_failed_signal()->is_pulsed());
        ASSERT_TRUE(callback.registrant != NULL);
        EXPECT_TRUE(callback.upgraded);

        /* Test writeread and read queries */

        std::string response = callback.registrant->writeread("wr1", repli_timestamp_t::invalid, osource.check_in("unittest"), &interruptor);
        EXPECT_EQ(response, "handle_writeread(wr1)");
        EXPECT_EQ(handler.log.size(), 2);
        EXPECT_EQ(handler.log[1], "wr1");

        std::string response2 = callback.registrant->read("r1", osource.check_in("unittest"), &interruptor);
        EXPECT_EQ(response2, "handle_read(r1)");
        EXPECT_EQ(handler.log.size(), 3);
        EXPECT_EQ(handler.log[2], "r1");

        /* Here registrant gets destroyed to test deregistration */
    }

    EXPECT_TRUE(callback.registrant == NULL);
}
TEST(ClusteringRegistration, Register) {
    run_in_thread_pool(&run_register_test);
}

}   /* namespace unittest */
