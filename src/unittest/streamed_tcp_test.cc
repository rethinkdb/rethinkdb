#include "unittest/gtest.hpp"

#include "arch/streamed_tcp.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

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
void run_in_thread_pool(boost::function<void()> fun) {
    thread_pool_t thread_pool(1, false /* TODO: Add --set-affinity option. */);
    starter_t starter(&thread_pool, fun);
    thread_pool.run(&starter);
}

void handle_echo_conn(boost::scoped_ptr<streamed_tcp_conn_t> &conn) {
    while (conn->get_istream().good()) {
        // Read a line and send it back
        std::string str;
        conn->get_istream() >> str;
        if (!conn->get_istream().fail()) {
            conn->get_ostream() << str << std::endl; // endl causes flush()
        }
    }
}

void handle_charwise_echo_conn(boost::scoped_ptr<streamed_tcp_conn_t> &conn) {
    while (conn->get_istream().good()) {
        // Read a char and send it back
        char c;
        conn->get_istream().read(&c, 1);
        if (!conn->get_istream().fail()) {
            conn->get_ostream().write(&c, 1).flush();
        }
    }
}

}   /* anonymous namespace */

void run_transmit_lines_test() {
    int port = 10000 + randint(20000);
    streamed_tcp_listener_t listener(port, boost::bind(&handle_echo_conn, _1));

    streamed_tcp_conn_t conn("127.0.0.1", port);
    std::string test_str = "0";
    for (int i = 0; i < 8192 / 32; ++i, test_str += std::string(32, static_cast<char>('a' + (i % 26)))) {
        conn.get_ostream() << test_str.substr(0, i);
        conn.get_ostream() << test_str.substr(i) << std::endl; // endl causes flush()
        //fprintf(stderr, "sent stuff of size %lu\n", test_str.length());
        std::string str;
        conn.get_istream() >> str;
        //fprintf(stderr, "got stuff\n");
        EXPECT_TRUE(!conn.get_istream().fail());
        EXPECT_EQ(str, test_str);
    }
    conn.get_ostream().flush();
}
TEST(TCPStreamTest, TransmitLines) {
    run_in_thread_pool(&run_transmit_lines_test);
}

void run_boost_serialize_test() {
    int port = 10000 + randint(20000);
    streamed_tcp_listener_t listener(port, boost::bind(&handle_charwise_echo_conn, _1));
    streamed_tcp_conn_t conn("127.0.0.1", port);

    for (int n = 0; n < 5; n++) {

        {
            int i = 123;
            boost::archive::binary_oarchive sender(conn.get_ostream());
            sender << i;
        }
    
        EXPECT_FALSE(conn.get_istream().fail());
        EXPECT_FALSE(conn.get_ostream().fail());
    
        {
            boost::archive::binary_iarchive receiver(conn.get_istream());
            int i;
            receiver >> i;
            EXPECT_EQ(i, 123);
        }
    
        EXPECT_FALSE(conn.get_istream().fail());
        EXPECT_FALSE(conn.get_ostream().fail());
    }
}
TEST(TCPStreamTest, BoostSerialize) {
    run_in_thread_pool(&run_boost_serialize_test);
}

}
