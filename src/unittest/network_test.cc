#include "arch/linux/arch.hpp"
#include "unittest/server_test_helper.hpp"
#include "arch/linux/coroutines.hpp"
#include <boost/crc.hpp>
#include <string>
#include <vector>
#include <cstdlib>

#include "unittest/gtest.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

struct network_tester_t {
public:
    thread_pool_t thread_pool;
    network_tester_t(size_t num_threads) : thread_pool(num_threads, false) { };

    void run() {
        struct starter_t : public thread_message_t {
            network_tester_t *network_test;
            starter_t(network_tester_t *network_test) : network_test(network_test) { }
            void on_thread_switch() {
                coro_t::spawn(boost::bind(&network_tester_t::run_tests, network_test));
            }
        } starter(this);

        thread_pool.run(&starter);
    }

private:
    void run_tests() {
        trace_call(test_read_write, false);
        trace_call(test_read_write, true);
        trace_call(thread_pool.shutdown);
    }

    std::string error;

    struct test_failed_exc_t : public std::exception {
        const char *what() throw() {
            return "test failed";
        }
    };

    void fail(const std::string& info) {
        // TODO: resource lock
        error = info;
        throw test_failed_exc_t();
    }

    struct connection_handler_t {
        cond_t ready;
        boost::scoped_ptr<tcp_conn_t> server;
        void add_connection(boost::scoped_ptr<tcp_conn_t>& new_connection) { rassert(server == NULL); server.swap(new_connection); ready.pulse(); };
    };

    void test_read_write(bool raw) {
        connection_handler_t connection_handler;
        uint16_t port = rand() % 64512 + 1024;
        logINF("network_test port: %d\n", port);

        tcp_listener_t listener(port, boost::bind(&connection_handler_t::add_connection, &connection_handler, _1));
        boost::scoped_ptr<tcp_conn_t> client(new tcp_conn_t("localhost", port));
        boost::scoped_ptr<tcp_conn_t> server;
        connection_handler.ready.wait();
        server.swap(connection_handler.server);

        // Unthread connections
        home_thread_mixin_t::rethread_t client_rethread(client.get(), INVALID_THREAD);
        home_thread_mixin_t::rethread_t server_rethread(server.get(), INVALID_THREAD);

        cond_t server_done;
        cond_t client_done;

        if (raw) {
        coro_t::spawn_on_thread(0, boost::bind(&network_tester_t::raw_read, this, &server->get_raw_connection(), &server_done));
        coro_t::spawn_on_thread(1, boost::bind(&network_tester_t::raw_write, this, &client->get_raw_connection(), &client_done));
        } else {
            coro_t::spawn_on_thread(0, boost::bind(&network_tester_t::buffered_read, this, server.get(), &server_done));
            coro_t::spawn_on_thread(1, boost::bind(&network_tester_t::buffered_write, this, client.get(), &client_done));

        }

        server_done.wait();
        client_done.wait();
    }

    static uint32_t get_crc(const std::vector<char>& buffer) {
        boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true> crc_computer;
        crc_computer.process_bytes(buffer.data(), buffer.size());
        return crc_computer.checksum();
    }

    void raw_read(raw_tcp_conn_t* conn, cond_t* done) {
        home_thread_mixin_t::rethread_t rethreader(conn, get_thread_id());

        try {
            std::vector<char> buffer;
            while (true) {
                size_t length;
                uint32_t crc;
                conn->read_exactly(&length, sizeof(length));
                buffer.resize(length);
                conn->read_exactly(buffer.data(), buffer.size());
                conn->read_exactly(&crc, sizeof(crc));

                if (get_crc(buffer) != crc)
                    fail("crc mismatch");
            }

        } catch (tcp_conn_t::read_closed_exc_t& ex) {
        } catch (test_failed_exc_t& ex) {
        } catch (...) {
            fail("unexpected exception in server");
        }
        done->pulse();
    }

    void raw_write(raw_tcp_conn_t* conn, cond_t* done) {
        home_thread_mixin_t::rethread_t rethreader(conn, get_thread_id());

        try {
            for (size_t buffer_size = 0; buffer_size < MEGABYTE; buffer_size += 512) {
                std::vector<char> buffer(buffer_size);
                for (uint32_t *i = reinterpret_cast<uint32_t *>(buffer.data());
                     reinterpret_cast<char *>(i) < reinterpret_cast<char *>(buffer.data()) + buffer_size; ++i) {
                    *i = rand();
                }
                uint32_t crc = get_crc(buffer);
                conn->write(&buffer_size, sizeof(buffer_size), NULL);
                size_t bytes_written = 0;
                while (bytes_written < buffer.size()) {
                    size_t write_now = rand() % (buffer.size() - bytes_written + 1);
                    conn->write(buffer.data() + bytes_written, write_now, NULL);
                    bytes_written += write_now;
                }
                conn->write(&crc, sizeof(crc), NULL);
            }

            conn->shutdown_write();
        } catch (tcp_conn_t::write_closed_exc_t& ex) {
            fail("client write side was closed");
        } catch (test_failed_exc_t& ex) {
        } catch (...) {
            fail("unexpected exception in client");
        }
        done->pulse();
    }

    void buffered_read(buffered_tcp_conn_t* conn, cond_t* done) {
        home_thread_mixin_t::rethread_t rethreader(conn, get_thread_id());

        try {
            std::vector<char> buffer;
            while (true) {
                size_t length;
                uint32_t crc;
                conn->read_exactly(&length, sizeof(length));
                buffer.resize(length);
                conn->read_exactly(buffer.data(), buffer.size());
                conn->read_exactly(&crc, sizeof(crc));

                if (get_crc(buffer) != crc)
                    fail("crc mismatch");
            }

            if (conn->is_read_open()) conn->shutdown_read();
        } catch (tcp_conn_t::read_closed_exc_t& ex) {
            if (conn->is_read_open()) conn->shutdown_read();
        } catch (test_failed_exc_t& ex) {
            if (conn->is_read_open()) conn->shutdown_read();
        } catch (...) {
            if (conn->is_read_open()) conn->shutdown_read();
            fail("unexpected exception in server");
        }
        done->pulse();
    }

    void buffered_write(buffered_tcp_conn_t* conn, cond_t* done) {
        home_thread_mixin_t::rethread_t rethreader(conn, get_thread_id());

        try {
            for (size_t buffer_size = 0; buffer_size < MEGABYTE; buffer_size += 512) {
                std::vector<char> buffer(buffer_size);
                for (uint32_t *i = reinterpret_cast<uint32_t *>(buffer.data());
                     reinterpret_cast<char *>(i) < reinterpret_cast<char *>(buffer.data()) + buffer_size; ++i) {
                    *i = rand();
                }
                uint32_t crc = get_crc(buffer);
                conn->write(&buffer_size, sizeof(buffer_size));
                size_t bytes_written = 0;
                while (bytes_written < buffer.size()) {
                    size_t write_now = rand() % (buffer.size() - bytes_written + 1);
                    conn->write(buffer.data() + bytes_written, write_now);
                    bytes_written += write_now;
                }
                conn->write(&crc, sizeof(crc));
            }

            // Getting the raw connection should flush the write buffer
            if (conn->is_write_open()) conn->shutdown_write();
        } catch (tcp_conn_t::write_closed_exc_t& ex) {
            fail("client write side was closed");
            if (conn->is_write_open()) conn->shutdown_write();
        } catch (test_failed_exc_t& ex) {
            if (conn->is_write_open()) conn->shutdown_write();
        } catch (...) {
            fail("unexpected exception in client");
            if (conn->is_write_open()) conn->shutdown_write();
        }
        done->pulse();
    }
};

TEST(NetworkTest, all_tests) {
    network_tester_t(2).run();
}

}  // namespace unittest
