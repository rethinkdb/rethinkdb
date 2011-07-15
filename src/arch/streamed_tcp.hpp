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

    // TODO: The following might already have equivalents in std::iostream, which we should
    // support instead.
    // However they are convenient for porting legacy code to the streamed_tcp_conn_t
    // interface.

    /* Call shutdown_read() to close the half of the pipe that goes from the peer to us. If there
    is an outstanding read() or peek_until() operation, it will throw read_closed_exc_t. */
    void shutdown_read() {
        conn_->shutdown_read();
    }

    /* Returns false if the half of the pipe that goes from the peer to us has been closed. */
    bool is_read_open() {
        return conn_->is_read_open();
    }

    /* Call shutdown_write() to close the half of the pipe that goes from us to the peer. If there
    is a write currently happening, it will get write_closed_exc_t. */
    void shutdown_write() {
        conn_->shutdown_write();
    }

    /* Returns false if the half of the pipe that goes from us to the peer has been closed. */
    bool is_write_open() {
        return conn_->is_write_open();
    }

private:
    /*
     This implements the actual streambuf object. It is buffered, so it's *not*
     safe to also use the read and write functionality of the underlying tcp_conn_t
     object at the same time. This is why we make this private to streamed_tcp_conn_t.
     */
    class tcp_conn_streambuf_t : public std::basic_streambuf<char, std::char_traits<char> > {
    public:
        tcp_conn_streambuf_t(tcp_conn_t *conn) : conn(conn) {
            // Initialize basic_streambuf, with gets being buffered and puts being unbuffered
            setg(&get_buf[0], &get_buf[0], &get_buf[0]);
            setp(0, 0);
            unsynced_write = false;
        }

    private:
        tcp_conn_t *conn;

    protected:
        // Implementation of basic_streambuf methods
        virtual int underflow() {
            if (gptr() >= egptr()) {
                // No data left in buffer, retrieve new data
                try {
                    // Read up to GET_BUF_LENGTH characters into get_buf
                    size_t bytes_read = conn->read_some(get_buf, GET_BUF_LENGTH);
                    rassert (bytes_read > 0);
                    setg(&get_buf[0], &get_buf[0], &get_buf[bytes_read]);
                } catch (tcp_conn_t::read_closed_exc_t &e) {
                    return std::char_traits<char>::eof();
                }
            }

            int i = std::char_traits<char>::to_int_type(*gptr());
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
                } catch (tcp_conn_t::write_closed_exc_t &e) {
                    return std::char_traits<char>::eof();
                }
                return std::char_traits<char>::not_eof(i);
            }
        }

        virtual int sync() {
            try {
                // Only flush if necessary...
                if (unsynced_write) {
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

        // Read buffer
        static const size_t GET_BUF_LENGTH = 512;
        char get_buf[GET_BUF_LENGTH];

        bool unsynced_write;
    };

    friend class streamed_tcp_listener_t;
    // Used by streamed_tcp_listener_t
    explicit streamed_tcp_conn_t(tcp_conn_t *conn) :
            std::iostream(&conn_streambuf),
            conn_streambuf(conn) {
        conn_.reset(conn);
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
            boost::function<void(streamed_tcp_conn_t*)> callback
        ) : callback(callback), listener(port, boost::bind(&streamed_tcp_listener_t::handle, this, _1) ) {
    }

private:
    // The wrapping handler
    void handle(linux_tcp_conn_t *conn) {
        streamed_tcp_conn_t *streamed_conn = new streamed_tcp_conn_t(conn);
        callback(streamed_conn);
    }

    // The callback to call when we get a connection
    boost::function<void(streamed_tcp_conn_t*)> callback;

    tcp_listener_t listener;
};

#endif
