// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/timing.hpp"
#include "clustering/administration/metadata.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/dummy_metadata_controller.hpp"
#include "unittest/unittest_utils.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/mailbox/typed.hpp"

namespace unittest {

namespace {

/* `dummy_mailbox_t` is a `raw_mailbox_t` that keeps track of messages it receives.
You can send to a `dummy_mailbox_t` with `send()`. */

struct dummy_mailbox_t {
private:
    std::set<int> inbox;

    class read_impl_t;

    class write_impl_t : public mailbox_write_callback_t {
    public:
        explicit write_impl_t(int _arg) : arg(_arg) { }
        void write(cluster_version_t cluster_version, write_message_t *wm) {
            serialize_for_version(cluster_version, wm, arg);
        }
#ifdef ENABLE_MESSAGE_PROFILER
        const char *message_profiler_tag() const {
            return "unittest";
        }
#endif
    private:
        friend class read_impl_t;
        int32_t arg;
    };

    class read_impl_t : public mailbox_read_callback_t {
    public:
        explicit read_impl_t(dummy_mailbox_t *_parent) : parent(_parent) { }
        void read(read_stream_t *stream, UNUSED signal_t *interruptor) {
            int i;
            archive_result_t res = deserialize<cluster_version_t::CLUSTER>(stream, &i);
            if (bad(res)) { throw fake_archive_exc_t(); }
            parent->inbox.insert(i);
        }
    private:
        dummy_mailbox_t *parent;
    };

    read_impl_t reader;
public:
    friend void send(mailbox_manager_t *, raw_mailbox_t::address_t, int);

    explicit dummy_mailbox_t(mailbox_manager_t *m) :
        reader(this), mailbox(m, &reader)
        { }
    void expect(int message) {
        EXPECT_EQ(1u, inbox.count(message));
    }
    raw_mailbox_t mailbox;
};

void send(mailbox_manager_t *c, raw_mailbox_t::address_t dest, int message) {
    dummy_mailbox_t::write_impl_t writer(message);
    send(c, dest, &writer);
}

}   /* anonymous namespace */

/* `MailboxStartStop` creates and destroys some mailboxes. */
TPTEST(RPCMailboxTest, MailboxStartStop, 2) {
    connectivity_cluster_t c;
    mailbox_manager_t m(&c, 'M');
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);
    connectivity_cluster_t::run_t r(&c, generate_uuid(), get_unittest_addresses(),
        peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());

    /* Make sure we can create a mailbox */
    dummy_mailbox_t mbox1(&m);

    /* Make sure we can create a mailbox on an arbitrary thread */
    on_thread_t thread_switcher((threadnum_t(1)));
    dummy_mailbox_t mbox2(&m);
}

/* `MailboxMessage` sends messages to some mailboxes */
TPTEST_MULTITHREAD(RPCMailboxTest, MailboxMessage, 3) {
    connectivity_cluster_t c1, c2;
    mailbox_manager_t m1(&c1, 'M'), m2(&c2, 'M');
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);
    connectivity_cluster_t::run_t r1(&c1, generate_uuid(), get_unittest_addresses(),
        peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t r2(&c2, generate_uuid(), get_unittest_addresses(),
        peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());
    r1.join(get_cluster_local_address(&c2));
    let_stuff_happen();

    /* Create a mailbox and send it three messages */
    dummy_mailbox_t mbox(&m1);
    raw_mailbox_t::address_t address = mbox.mailbox.get_address();

    send(&m1, address, 88555);
    send(&m2, address, 3131);
    send(&m1, address, 7);

    let_stuff_happen();

    mbox.expect(88555);
    mbox.expect(3131);
    mbox.expect(7);
}

/* `DeadMailbox` sends a message to a defunct mailbox. The expected behavior is
for the message to be silently ignored. */
TPTEST_MULTITHREAD(RPCMailboxTest, DeadMailbox, 3) {
    connectivity_cluster_t c1, c2;
    mailbox_manager_t m1(&c1, 'M'), m2(&c2, 'M');
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);
    connectivity_cluster_t::run_t r1(&c1, generate_uuid(), get_unittest_addresses(),
        peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());
    connectivity_cluster_t::run_t r2(&c2, generate_uuid(), get_unittest_addresses(),
        peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());
    /* Create a mailbox, take its address, then destroy it. */
    raw_mailbox_t::address_t address;
    {
        dummy_mailbox_t mbox(&m1);
        address = mbox.mailbox.get_address();
    }

    send(&m1, address, 12345);
    send(&m2, address, 78888);

    let_stuff_happen();
}
/* `MailboxAddressSemantics` makes sure that `raw_mailbox_t::address_t` behaves as
expected. */
TPTEST_MULTITHREAD(RPCMailboxTest, MailboxAddressSemantics, 3) {
    raw_mailbox_t::address_t nil_addr;
    EXPECT_TRUE(nil_addr.is_nil());

    connectivity_cluster_t c;
    mailbox_manager_t m(&c, 'M');
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);
    connectivity_cluster_t::run_t r(&c, generate_uuid(), get_unittest_addresses(),
        peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());

    dummy_mailbox_t mbox(&m);
    raw_mailbox_t::address_t mbox_addr = mbox.mailbox.get_address();
    EXPECT_FALSE(mbox_addr.is_nil());
    EXPECT_TRUE(mbox_addr.get_peer() == c.get_me());
}

/* `TypedMailbox` makes sure that `mailbox_t<>` works. */

void string_push_back(std::vector<std::string> *v, const std::string &pushee) {
    v->push_back(pushee);
}

TPTEST_MULTITHREAD(RPCMailboxTest, TypedMailbox, 3) {
    connectivity_cluster_t c;
    mailbox_manager_t m(&c, 'M');
    heartbeat_semilattice_metadata_t heartbeat_semilattice_metadata;
    dummy_semilattice_controller_t<heartbeat_semilattice_metadata_t>
        heartbeat_manager(heartbeat_semilattice_metadata);
    connectivity_cluster_t::run_t r(&c, generate_uuid(), get_unittest_addresses(),
        peer_address_t(), ANY_PORT, 0, heartbeat_manager.get_view());

    std::vector<std::string> inbox;
    mailbox_t<void(std::string)> mbox(&m,
        [&](signal_t *, const std::string &str) {
            inbox.push_back(str);
        });

    mailbox_addr_t<void(std::string)> addr = mbox.get_address();

    send(&m, addr, std::string("foo"));
    send(&m, addr, std::string("bar"));
    send(&m, addr, std::string("baz"));

    let_stuff_happen();

    EXPECT_EQ(3u, inbox.size());
    if (inbox.size() == 3) {
        EXPECT_EQ(inbox[0], "foo");
        EXPECT_EQ(inbox[1], "bar");
        EXPECT_EQ(inbox[2], "baz");
    }
}

}   /* namespace unittest */
