#ifndef __ARCH_STREAMED_TCP_HPP__
#define __ARCH_STREAMED_TCP_HPP__

/* This file contains an std::streambuf implementation that works 
with a tcp_conn_t object. This allows us to create std::ostream and std::istream
objects that interact directly with a tcp_conn_t TCP connection. For example
boost serialize requires streams for serializing data. 
This implementation does read buffering itself (where the buffering logic is
inherited from basic_streambuf). This is mostly because linux_tcp_conn_t::pop
is badly implemented and basic_streambuf has that logic implemented anyway.
Eventually we should rather fix buffering in the tcp_conn_t implementation and
make this synchronous. */

#include <iostream>
#include <boost/scoped_ptr.hpp>
#include <streambuf>
#include "arch/arch.hpp"

/*
 This implements the actual streambuf object. It is unbuffered, so that it's
 safe to also use the read and write functionality of the underlying tcp_conn_t
 object.
 */

class tcp_conn_streambuf_t : public std::basic_streambuf<char, std::char_traits<char> > {
public:
    tcp_conn_streambuf_t(tcp_conn_t *conn) : conn(conn) {
        // Initialize basic_streambuf, with both gets and puts being unbuffered
        setg(0, 0, 0);
        setp(0, 0);
        unsynced_write = false;
    }

private:
    tcp_conn_t *conn;

protected:
    // Implementation of basic_streambuf methods
    virtual int underflow() {
        try {
            const_charslice cs = conn->peek(1);
            rassert(cs.end > cs.beg, "Didn't get data from peek(1). This is weird.");
            input_buf = cs.beg[0];
            conn->pop(1);
            setg(&input_buf, &input_buf, &input_buf+1);
            fprintf(stderr, "G: %c\n", input_buf);
        } catch (tcp_conn_t::read_closed_exc_t &e) {
            return std::char_traits<char>::eof();
        }

        int i = std::char_traits<char>::to_int_type(input_buf);
        return std::char_traits<char>::not_eof(i);
    }

    virtual int overflow(int i = std::char_traits<char>::eof()) {
        if (!conn->is_write_open()) {
            return std::char_traits<char>::eof();
        } else if (i == std::char_traits<char>::eof()) {
            return std::char_traits<char>::not_eof(i);
        } else {
            char c = static_cast<char>(i);
            rassert(static_cast<int>(c) == i);
            try {
                unsynced_write = true;
                conn->write_buffered(&c, 1);
                fprintf(stderr, "S: %c\n", c);
            } catch (tcp_conn_t::write_closed_exc_t &e) {
                return std::char_traits<char>::eof();
            }
            return std::char_traits<char>::not_eof(i);
        }
    }

    virtual int sync() {
        try {
            if (unsynced_write) {
                // Hack so we don't flush if we are used for reads only. Otherwise
                // we can run into problems because tcp_conn_t allows only one
                // active write or flush at a time.
                conn->flush_buffer();
                unsynced_write = false;
            }
            return 0;
        } catch (tcp_conn_t::write_closed_exc_t &e) {
            return -1;
        }
    }

private:
    DISABLE_COPYING(tcp_conn_streambuf_t);

    char input_buf;

    bool unsynced_write;
};


/*
 streamed_tcp_conn_t provides an input and output stream around a
 tcp_conn_t.
 In the long term, this might become a replacement for the public tcp_conn_t
 interface.
*/

class streamed_tcp_conn_t : public std::iostream {
public:
    streamed_tcp_conn_t(const char *host, int port) :
            std::iostream(&conn_streambuf),
            conn_(new tcp_conn_t(host, port)),
            conn_streambuf(conn_.get()) {
    }
    streamed_tcp_conn_t(const ip_address_t &host, int port) :
            std::iostream(&conn_streambuf),
            conn_(new tcp_conn_t(host, port)),
            conn_streambuf(conn_.get()) {
    }

    tcp_conn_t *get_raw_conn() {
        /* Thanks to tcp_conn_streambuf_t being unbuffered with respect to
        tcp_conn_t, it is safe to pass out the tcp_conn_t. */
        rassert(conn_.get());
        return conn_.get();
    }


private:
    friend class streamed_tcp_listener_t;
    // Used by streamed_tcp_listener_t
    explicit streamed_tcp_conn_t(boost::scoped_ptr<tcp_conn_t> &conn) :
            std::iostream(&conn_streambuf),
            conn_streambuf(conn.get()) {
        conn_.swap(conn);
    }

    boost::scoped_ptr<tcp_conn_t> conn_;
    tcp_conn_streambuf_t conn_streambuf;
};

/*
 tcp_listener_t creates tcp_conn_t objects. We want to be able to get
 streamed_tcp_conn_ts out of it. The following class provides a safe
 wrapper around tcp_listener_t which does exactly that.
*/

class streamed_tcp_listener_t {
public:
    streamed_tcp_listener_t(
            int port,
            boost::function<void(boost::scoped_ptr<streamed_tcp_conn_t>&)> callback
        ) : callback(callback), listener(port, boost::bind(&streamed_tcp_listener_t::handle, this, _1) ) {
    }

private:
    // The wrapping handler
    void handle(boost::scoped_ptr<linux_tcp_conn_t> &conn) {
        boost::scoped_ptr<streamed_tcp_conn_t> streamed_conn(new streamed_tcp_conn_t(conn));
        callback(streamed_conn);
    }

    // The callback to call when we get a connection
    boost::function<void(boost::scoped_ptr<streamed_tcp_conn_t>&)> callback;

    tcp_listener_t listener;
};

#endif
