// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "arch/timing.hpp"
#include "mock/unittest_utils.hpp"
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
        void write(write_stream_t *stream) {
            write_message_t msg;
            msg << arg;
            int res = send_write_message(stream, &msg);
            if (res) { throw fake_archive_exc_t(); }
        }
    private:
        friend class read_impl_t;
        int32_t arg;
    };

    class read_impl_t : public mailbox_read_callback_t {
    public:
        explicit read_impl_t(dummy_mailbox_t *_parent) : parent(_parent) { }
        void read(read_stream_t *stream) {
            int i;
            int res = deserialize(stream, &i);
            if (res) { throw fake_archive_exc_t(); }
            parent->inbox.insert(i);
        }
        void read(mailbox_write_callback_t *_writer) {
            write_impl_t *writer = static_cast<write_impl_t*>(_writer);
            parent->inbox.insert(writer->arg);
        }
    private:
        dummy_mailbox_t *parent;
    };

    read_impl_t reader;
public:
    friend void send(mailbox_manager_t *, raw_mailbox_t::address_t, int);

    explicit dummy_mailbox_t(mailbox_manager_t *m) :
        reader(this), mailbox(m, mailbox_home_thread, &reader)
        { }
    void expect(int message) {
        EXPECT_EQ(1, inbox.count(message));
    }
    raw_mailbox_t mailbox;
};

void send(mailbox_manager_t *c, raw_mailbox_t::address_t dest, int message) {
    dummy_mailbox_t::write_impl_t writer(message);
    send(c, dest, &writer);
}

}   /* anonymous namespace */

/* `MailboxStartStop` creates and destroys some mailboxes. */

void run_mailbox_start_stop_test() {
    int port = mock::randport();
    connectivity_cluster_t c;
    mailbox_manager_t m(&c);
    connectivity_cluster_t::run_t r(&c, port, &m);

    /* Make sure we can create a mailbox */
    dummy_mailbox_t mbox1(&m);

    /* Make sure we can create a mailbox on an arbitrary thread */
    on_thread_t thread_switcher(1);
    dummy_mailbox_t mbox2(&m);
}
TEST(RPCMailboxTest, MailboxStartStop) {
    mock::run_in_thread_pool(&run_mailbox_start_stop_test, 2);
}

/* `MailboxMessage` sends messages to some mailboxes */

void run_mailbox_message_test() {
    int port = mock::randport();
    connectivity_cluster_t c1, c2;
    mailbox_manager_t m1(&c1), m2(&c2);
    connectivity_cluster_t::run_t r1(&c1, port, &m1), r2(&c2, port+1, &m2);
    r1.join(c2.get_peer_address(c2.get_me()));
    mock::let_stuff_happen();

    /* Create a mailbox and send it three messages */
    dummy_mailbox_t mbox(&m1);
    raw_mailbox_t::address_t address = mbox.mailbox.get_address();

    send(&m1, address, 88555);
    send(&m2, address, 3131);
    send(&m1, address, 7);

    mock::let_stuff_happen();

    mbox.expect(88555);
    mbox.expect(3131);
    mbox.expect(7);
}
TEST(RPCMailboxTest, MailboxMessage) {
    mock::run_in_thread_pool(&run_mailbox_message_test);
}

TEST(RPCMailboxTest, MailboxMessageMultiThread) {
    mock::run_in_thread_pool(&run_mailbox_message_test, 3);
}

/* `DeadMailbox` sends a message to a defunct mailbox. The expected behavior is
for the message to be silently ignored. */

void run_dead_mailbox_test() {
    int port = mock::randport();
    connectivity_cluster_t c1, c2;
    mailbox_manager_t m1(&c1), m2(&c2);
    connectivity_cluster_t::run_t r1(&c1, port, &m1), r2(&c2, port+1, &m2);

    /* Create a mailbox, take its address, then destroy it. */
    raw_mailbox_t::address_t address;
    {
        dummy_mailbox_t mbox(&m1);
        address = mbox.mailbox.get_address();
    }

    send(&m1, address, 12345);
    send(&m2, address, 78888);

    mock::let_stuff_happen();
}
TEST(RPCMailboxTest, DeadMailbox) {
    mock::run_in_thread_pool(&run_dead_mailbox_test);
}
TEST(RPCMailboxTest, DeadMailboxMultiThread) {
    mock::run_in_thread_pool(&run_dead_mailbox_test, 3);
}
/* `MailboxAddressSemantics` makes sure that `raw_mailbox_t::address_t` behaves as
expected. */

void run_mailbox_address_semantics_test() {

    raw_mailbox_t::address_t nil_addr;
    EXPECT_TRUE(nil_addr.is_nil());

    int port = mock::randport();
    connectivity_cluster_t c;
    mailbox_manager_t m(&c);
    connectivity_cluster_t::run_t r(&c, port, &m);

    dummy_mailbox_t mbox(&m);
    raw_mailbox_t::address_t mbox_addr = mbox.mailbox.get_address();
    EXPECT_FALSE(mbox_addr.is_nil());
    EXPECT_TRUE(mbox_addr.get_peer() == c.get_me());
}
TEST(RPCMailboxTest, MailboxAddressSemantics) {
    mock::run_in_thread_pool(&run_mailbox_address_semantics_test);
}
TEST(RPCMailboxTest, MailboxAddressSemanticsMultiThread) {
    mock::run_in_thread_pool(&run_mailbox_address_semantics_test, 3);
}

/* `TypedMailbox` makes sure that `mailbox_t<>` works. */

void run_typed_mailbox_test() {

    int port = mock::randport();
    connectivity_cluster_t c;
    mailbox_manager_t m(&c);
    connectivity_cluster_t::run_t r(&c, port, &m);

    std::vector<std::string> inbox;
    mailbox_t<void(std::string)> mbox(&m, boost::bind(&std::vector<std::string>::push_back, &inbox, _1), mailbox_callback_mode_inline);

    mailbox_addr_t<void(std::string)> addr = mbox.get_address();

    send(&m, addr, std::string("foo"));
    send(&m, addr, std::string("bar"));
    send(&m, addr, std::string("baz"));

    mock::let_stuff_happen();

    EXPECT_EQ(inbox.size(), 3);
    if (inbox.size() == 3) {
        EXPECT_EQ(inbox[0], "foo");
        EXPECT_EQ(inbox[1], "bar");
        EXPECT_EQ(inbox[2], "baz");
    }
}
TEST(RPCMailboxTest, TypedMailbox) {
    mock::run_in_thread_pool(&run_typed_mailbox_test);
}
TEST(RPCMailboxTest, TypedMailboxMultiThread) {
    mock::run_in_thread_pool(&run_typed_mailbox_test, 3);
}

}   /* namespace unittest */
