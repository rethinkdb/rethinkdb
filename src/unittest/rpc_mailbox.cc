#include "unittest/gtest.hpp"

#include "rpc/mailbox/mailbox.hpp"
#include "rpc/mailbox/typed.hpp"

namespace unittest {

namespace {

/* `run_in_thread_pool()` starts a RethinkDB IO layer thread pool and calls the
given function within a coroutine inside of it. */

struct starter_t : public thread_message_t {
    thread_pool_t *tp;
    boost::function<void()> fun;
    starter_t(thread_pool_t *tp, boost::function<void()> fun) : tp(tp), fun(fun) { }
    void run() {
        fun();
        tp->shutdown();
    }
    void on_thread_switch() {
        coro_t::spawn_now(boost::bind(&starter_t::run, this));
    }
};
void run_in_thread_pool(boost::function<void()> fun, int nthreads = 1) {
    thread_pool_t thread_pool(nthreads);
    starter_t starter(&thread_pool, fun);
    thread_pool.run(&starter);
}

/* `let_stuff_happen()` delays for some time to let events occur */
void let_stuff_happen() {
    nap(1000);
}

/* `dummy_cluster_t` is a `mailbox_cluster_t` that keeps track of utility
messages it receives from other nodes in the cluster. `dummy_mailbox_t` is a
`mailbox_t` that keeps track of messages it receives. You can send to a
`dummy_mailbox_t` with `send()`. */

void write_integer(int i, std::ostream &stream) {
    stream << i;
}

struct dummy_cluster_t : public mailbox_cluster_t {
private:
    std::map<int, peer_id_t> inbox;
    void on_utility_message(peer_id_t peer, std::istream &stream, boost::function<void()> &on_done) {
        int i;
        stream >> i;
        on_done();
        inbox[i] = peer;
    }
public:
    dummy_cluster_t(int i) : mailbox_cluster_t(i) { }
    void send(int message, peer_id_t peer) {
        send_utility_message(peer, boost::bind(&write_integer, message, _1));
    }
    void expect(int message, peer_id_t peer) {
        EXPECT_TRUE(inbox.find(message) != inbox.end());
        EXPECT_TRUE(inbox[message] == peer);
    }
};

struct dummy_mailbox_t : public mailbox_t {
private:
    std::set<int> inbox;
    void on_message(std::istream &stream, boost::function<void()> &on_done) {
        int i;
        stream >> i;
        on_done();
        inbox.insert(i);
    }
public:
    dummy_mailbox_t(mailbox_cluster_t *c) : mailbox_t(c) { }
    void expect(int message) {
        EXPECT_TRUE(inbox.count(message) == 1);
    }
};

void send(mailbox_cluster_t *c, dummy_mailbox_t::address_t dest, int message) {
    send(c, dest, boost::bind(&write_integer, message, _1));
}

}   /* anonymous namespace */

/* `UtilityMessage` sends some utility messages between the nodes of a cluster.
It's just a clone of `Message` in `rpc_connectivity.cc`, except with
`mailbox_cluster_t`'s utility message mechanism instead of
`connectivity_cluster_t`'s message mechanism. */

void run_utility_message_test() {
    int port = 10000 + rand() % 20000;
    dummy_cluster_t c1(port), c2(port+1), c3(port+2);
    c2.join(peer_address_t(ip_address_t::us(), port));
    c3.join(peer_address_t(ip_address_t::us(), port));

    let_stuff_happen();

    c1.send(873, c2.get_me());
    c2.send(66663, c1.get_me());
    c3.send(6849, c1.get_me());
    c3.send(999, c3.get_me());

    let_stuff_happen();

    c2.expect(873, c1.get_me());
    c1.expect(66663, c2.get_me());
    c1.expect(6849, c3.get_me());
    c3.expect(999, c3.get_me());
}
TEST(RPCMailboxTest, UtilityMessage) {
    run_in_thread_pool(&run_utility_message_test);
}

/* `MailboxStartStop` creates and destroys some mailboxes. */

void run_mailbox_start_stop_test() {
    int port = 10000 + rand() % 20000;
    dummy_cluster_t cluster(port);

    /* Make sure we can create a mailbox */
    dummy_mailbox_t mbox1(&cluster);

    /* Make sure we can create a mailbox on an arbitrary thread */
    on_thread_t thread_switcher(1);
    dummy_mailbox_t mbox2(&cluster);
}
TEST(RPCMailboxTest, MailboxStartStop) {
    run_in_thread_pool(&run_mailbox_start_stop_test, 2);
}

/* `MailboxMessage` sends messages to some mailboxes */

void run_mailbox_message_test() {
    int port = 10000 + rand() % 20000;
    dummy_cluster_t cluster1(port), cluster2(port+1);
    cluster1.join(peer_address_t(ip_address_t::us(), port+1));
    let_stuff_happen();

    /* Create a mailbox and send it three messages */
    dummy_mailbox_t mbox(&cluster1);
    mailbox_t::address_t address(&mbox);

    send(&cluster1, address, 88555);
    send(&cluster2, address, 3131);
    send(&cluster1, address, 7);

    let_stuff_happen();

    mbox.expect(88555);
    mbox.expect(3131);
    mbox.expect(7);
}
TEST(RPCMailboxTest, MailboxMessage) {
    run_in_thread_pool(&run_mailbox_message_test);
}

/* `DeadMailbox` sends a message to a defunct mailbox. The expected behavior is
for the message to be silently ignored. */

void run_dead_mailbox_test() {
    int port = 10000 + rand() % 20000;
    dummy_cluster_t cluster1(port), cluster2(port+1);

    /* Create a mailbox, take its address, then destroy it. */
    mailbox_t::address_t address;
    {
        dummy_mailbox_t mbox(&cluster1);
        address = mailbox_t::address_t(&mbox);
    }

    send(&cluster1, address, 12345);
    send(&cluster2, address, 78888);

    let_stuff_happen();
}
TEST(RPCMailboxTest, DeadMailbox) {
    run_in_thread_pool(&run_dead_mailbox_test);
}

/* `MailboxAddressSemantics` makes sure that `mailbox_t::address_t` behaves as
expected. */

void run_mailbox_address_semantics_test() {

    mailbox_t::address_t nil_addr;
    EXPECT_TRUE(nil_addr.is_nil());

    int port = 10000 + rand() % 20000;
    dummy_cluster_t cluster(port);

    dummy_mailbox_t mbox(&cluster);
    mailbox_t::address_t mbox_addr(&mbox);
    EXPECT_FALSE(mbox_addr.is_nil());
    EXPECT_TRUE(mbox_addr.get_peer() == cluster.get_me());
}
TEST(RPCMailboxTest, MailboxAddressSemantics) {
    run_in_thread_pool(&run_mailbox_address_semantics_test);
}

/* `TypedMailbox` makes sure that `async_mailbox_t<>` works. */

void run_typed_mailbox_test() {

    int port = 10000 + rand() % 20000;
    dummy_cluster_t cluster(port);

    std::vector<std::string> inbox;
    async_mailbox_t<void(std::string)> mbox(&cluster, boost::bind(&std::vector<std::string>::push_back, &inbox, _1));

    async_mailbox_t<void(std::string)>::address_t addr(&mbox);

    send(&cluster, addr, std::string("foo"));
    send(&cluster, addr, std::string("bar"));
    send(&cluster, addr, std::string("baz"));

    let_stuff_happen();

    EXPECT_EQ(inbox.size(), 3);
    if (inbox.size() == 3) {
        EXPECT_EQ(inbox[0], "foo");
        EXPECT_EQ(inbox[1], "bar");
        EXPECT_EQ(inbox[2], "baz");
    }
}
TEST(RPCMailboxTest, TypedMailbox) {
    run_in_thread_pool(&run_typed_mailbox_test);
}

}   /* namespace unittest */
