// Copyright 2010-2012 RethinkDB, all rights reserved.
#include <ctime>
#include <functional>

#include "containers/object_buffer.hpp"
#include "concurrency/wait_any.hpp"
#include "arch/runtime/coroutines.hpp"
#include "arch/types.hpp"
#include "arch/timing.hpp"
#include "arch/io/network.hpp"
#include "unittest/gtest.hpp"
#include "http/http.hpp"
#include "http/routing_app.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

TEST(Http, FormatTime) {

    std::string formatted_time = http_format_date(1356998463);

    EXPECT_EQ("Tue, 01 Jan 2013 00:01:03 GMT", formatted_time);

}

// Helper functions to create dummy objects
http_req_t http_req_encoding(const std::string &encoding) {
    http_req_t req;
    req.add_header_line("Accept-Encoding", encoding);
    return req;
}

void test_encoding(const std::string &encoding, bool expected) {
    // Use at least 2k so compression has something to work with
    static std::string body;
    if (body.empty()) {
        for (size_t i = 0; i < 2048; ++i) {
            body += 'a' + (i % 26);
        }
    }

    http_req_t req = http_req_encoding(encoding);
    http_res_t res(http_status_code_t::OK);
    res.set_body("application/text", body);

    EXPECT_EQ(expected, maybe_gzip_response(req, &res)) << "Incorrect handling of encoding '" + encoding + "'";
}

TEST(Http, Encodings) {
    // These encodings are valid and should result in compression
    test_encoding("gzip", true);
    test_encoding("gzip", true);
    test_encoding("gzip ;q=001", true);
    test_encoding("gzip;q =0.1", true);
    test_encoding("gzip  ;q= 1000.11111  ", true);
    test_encoding("  identity;q=1000.1111  ,  gzip;q=1000.11111", true);
    test_encoding("compress, gzip", true);
    test_encoding("*", true);
    test_encoding("compress;q=0.5, gzip;q=1.0", true);
    test_encoding("gzip;q=1.0, identity; q=0.5, *;q=0", true);
    test_encoding("geewhiz;q=1.0,geezip;q=2.0,*;q=0.5", true);
    test_encoding("geewhiz;q=0.0,geezip;q=2.2,*;q=3", true);

    // These encodings are valid but should not result in compression
    test_encoding("", false);
    test_encoding("*;q=0.0", false);
    test_encoding("gzip;q=0.1,identity;q=0.2", false);
    test_encoding("gzip;q=1,*;q=1.84", false);
    test_encoding("identity;q=1,*;q=0.5", false);
    test_encoding("geewhiz", false);
    test_encoding("geewhiz;q=1.0", false);
    test_encoding("geewhiz;q=1.0,geezip;q=2.0", false);

    // These encodings have bad syntax and should fail
    test_encoding("gzip:q=0.1", false);
    test_encoding("gzip;q=0 .1", false);
    test_encoding("gzip;q=0. 1", false);
    test_encoding("gzip:q=0x1", false);
    test_encoding("gzip;q=", false);
    test_encoding("gzip;q", false);
    test_encoding("gzip;=0.5", false);
    test_encoding("gzip=9.9", false);
    test_encoding("gzip;", false);
    test_encoding("gzip;s=0.6", false);
    test_encoding("gzip;deflate", false);
    test_encoding("gzip,deflate,*=0.0", false);
    test_encoding("g^zip", false);
    test_encoding("g_zip", false);
}

class dummy_http_app_t : public http_app_t {
public:
    signal_t *get_handle_signal() {
        return &request_received;
    }

    void handle(const http_req_t &, http_res_t *, signal_t *interruptor) {
        request_received.pulse();
        interruptor->wait();
        throw interrupted_exc_t();
    }

private:
    // Signal used to synchronize between test and app_t
    cond_t request_received;
};

class http_interrupt_test_t {
public:
    http_interrupt_test_t(const std::string &_http_get_path,
                          const std::string &_http_get_body) :
        http_get_path(_http_get_path), http_get_body(_http_get_body) { }
    virtual ~http_interrupt_test_t() { }

    bool run() {
        http_app_t *app = create_app();

        ip_address_t loopback("127.0.0.1");
        std::set<ip_address_t> ip_addresses;
        ip_addresses.insert(loopback);
        server.create(nullptr, ip_addresses, 0, app);

        // Start an http request
        coro_t::spawn_sometime(std::bind(&http_interrupt_test_t::http_get, this));
        wait_for_connect();

        // Interrupt the request by destroying the server
        coro_t::spawn_sometime(std::bind(&http_interrupt_test_t::destruct_server, this));

        // Make sure the get had no reply
        wait_any_t success_fail(&http_get_timed_out, &http_get_succeeded);
        success_fail.wait();
        return http_get_timed_out.is_pulsed();
    }

protected:
    virtual http_app_t *create_app() = 0;
    virtual void wait_for_connect() = 0;

private:
    void destruct_server() {
        server.reset();
    }

    void http_get() {
        cond_t non_interruptor;
        ip_address_t loopback("127.0.0.1");
        tcp_conn_t http_conn(loopback, server->get_port(), &non_interruptor);

        // Send the get request
        std::string buffer = strprintf("GET %s HTTP/1.1\r\n\r\n%s\r\n",
                                       http_get_path.c_str(), http_get_body.c_str());
        http_conn.write(buffer.c_str(), buffer.length(), &non_interruptor);

        // Verify that we do not get a response - allow 0.5s
        signal_timer_t timeout;
        timeout.start(500);

        try {
            char dummy_buffer[1];
            http_conn.read(&dummy_buffer, sizeof(dummy_buffer), &timeout);
            http_get_succeeded.pulse();
            return;
        } catch (const tcp_conn_read_closed_exc_t &ex) {
            // This is expected, the server should not send a reply when shutting down
        }

        http_get_timed_out.pulse();
    }

    object_buffer_t<http_server_t> server;
    const std::string http_get_path;
    const std::string http_get_body;
    cond_t http_get_timed_out;
    cond_t http_get_succeeded;
};

class routing_app_interrupt_test_t : public http_interrupt_test_t {
public:
    explicit routing_app_interrupt_test_t(const std::string& http_get_path) :
        http_interrupt_test_t(http_get_path, "") { }
    virtual ~routing_app_interrupt_test_t() { }

private:
    dummy_http_app_t dummy_default;
    dummy_http_app_t dummy_b;
    object_buffer_t<routing_http_app_t> router;

    http_app_t *create_app() {
        std::map<std::string, http_app_t *> subroutes;
        subroutes.insert(std::make_pair("b", &dummy_b));

        router.create(&dummy_default, subroutes);
        return router.get();
    }

    void wait_for_connect() {
        wait_any_t waiter(dummy_default.get_handle_signal(), dummy_b.get_handle_signal());
        waiter.wait();
    }
};

TPTEST(Http, InterruptRoutingApp) {
    {
        routing_app_interrupt_test_t router_test("/");
        EXPECT_TRUE(router_test.run());
    }

    {
        routing_app_interrupt_test_t router_test("/b");
        EXPECT_TRUE(router_test.run());
    }
}

}  // namespace unittest
