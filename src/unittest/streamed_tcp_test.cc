#include "unittest/gtest.hpp"

#include "arch/streamed_tcp.hpp"

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
    thread_pool_t thread_pool(1);
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

}   /* anonymous namespace */

void run_tcp_stream_test() {
    int port = 10000 + rand() % 20000;
    streamed_tcp_listener_t listener(port, boost::bind(&handle_echo_conn, _1));

    streamed_tcp_conn_t conn("localhost", port);
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
TEST(TCPStreamTest, StartStop) {
    run_in_thread_pool(&run_tcp_stream_test);
}

}