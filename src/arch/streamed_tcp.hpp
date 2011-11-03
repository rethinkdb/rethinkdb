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

#include <istream>
#include <ostream>
#include <boost/scoped_ptr.hpp>
#include <streambuf>
#include "arch/arch.hpp"
#include <boost/bind.hpp>

/*
 streamed_tcp_conn_t provides an input and output stream around a
 tcp_conn_t.
 In the long term, this might become a replacement for the public tcp_conn_t
 interface.

 There is a reason for why we don't derive from std::istream and std::ostream
 directly: Both classes inherit virtually from std::ios. However we want
 their sync()s to behave differently, namely it should be a no-op for the istream
 and actually flush changed data out for the ostream.
 Now because both have virtual inheritance from std::ios, they would share a
 streambuf pointer, therefore making it impossible to provide different
 implementations of sync().
*/

class streamed_tcp_conn_t {
public:
    streamed_tcp_conn_t(const char *host, int port) :
            conn_(new tcp_conn_t(host, port)),
            conn_streambuf_in(conn_.get()),
            conn_streambuf_out(conn_.get()),
            istream(&conn_streambuf_in),
            ostream(&conn_streambuf_out) {
    }
    streamed_tcp_conn_t(const ip_address_t &host, int port) :
            conn_(new tcp_conn_t(host, port)),
            conn_streambuf_in(conn_.get()),
            conn_streambuf_out(conn_.get()),
            istream(&conn_streambuf_in),
            ostream(&conn_streambuf_out) {
    }

    std::istream &get_istream() {
        return istream;
    }
    std::ostream &get_ostream() {
        return ostream;
    }

    // TODO: The following might already have equivalents in std::iostream, which we should
    // support instead.
    // However they are convenient for porting legacy code to the streamed_tcp_conn_t
    // interface (which is even more legacy).

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

     Additionally, we split the streambuf up into an input and output streambuf.
     This is so that the input streambuf can ignore calls to sync(), which would
     interfere with sync()s on the output streambuf if both were the same.
     */
    class tcp_conn_streambuf_in_t : public std::basic_streambuf<char, std::char_traits<char> > {
    public:
        explicit tcp_conn_streambuf_in_t(tcp_conn_t *_conn) : conn(_conn) {
            // Initialize basic_streambuf, with gets being buffered and puts being unbuffered
            setg(&get_buf[0], &get_buf[0], &get_buf[0]);
            setp(0, 0);
        }

    protected:
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

        virtual int overflow(UNUSED int i = std::char_traits<char>::eof()) {
            fprintf(stderr, "overflow called on in stream!\n");
            return std::char_traits<char>::eof();
        }

        virtual int sync() {
            // There's nothing to do for input streams
            return 0;
        }

    private:
        DISABLE_COPYING(tcp_conn_streambuf_in_t);

        tcp_conn_t *conn;

        // Read buffer
        static const size_t GET_BUF_LENGTH = 512;
        char get_buf[GET_BUF_LENGTH];
    };

    class tcp_conn_streambuf_out_t : public std::basic_streambuf<char, std::char_traits<char> > {
    public:
        explicit tcp_conn_streambuf_out_t(tcp_conn_t *_conn) : conn(_conn) {
            setg(0, 0, 0);
            setp(&put_buf[0], &put_buf[PUT_BUF_LENGTH]);
            we_are_syncing = false;
        }

    protected:
        virtual int underflow() {
            crash("underflow called on out stream!\n");
            fprintf(stderr, "underflow called on out stream!\n");
            return std::char_traits<char>::eof();
        }

        virtual int overflow(int i = std::char_traits<char>::eof()) {
            if (!conn->is_write_open()) {
                return std::char_traits<char>::eof();
            } else if (i == std::char_traits<char>::eof()) {
                return std::char_traits<char>::not_eof(i);
            } else {
                // First flush out the current buffer and then put c on the buffer.
                if (sync() != 0) {
                    return std::char_traits<char>::eof();
                }
                char c = std::char_traits<char>::to_char_type(i);
                rassert(std::char_traits<char>::to_int_type(c) == i, "cannot safely cast %d to char and back", i);
                rassert(pptr() < epptr()); // We just ran sync(), so there should be space
                *pptr() = c;
                pbump(1);
                return std::char_traits<char>::not_eof(i);
            }
        }

        virtual int sync() {
            rassert(!we_are_syncing);
            we_are_syncing = true;
            try {
                rassert(pptr() >= pbase());
                char *pptr_when_starting = pptr();
                size_t size = static_cast<size_t>(pptr_when_starting - pbase());
                // Only flush if necessary...
                if (size > 0) {
                    conn->write(pbase(), size);
                    rassert(pptr() == pptr_when_starting, "Write happened while syncing?!");
                    setp(&put_buf[0], &put_buf[PUT_BUF_LENGTH]);
                }
                we_are_syncing = false;
                return 0;
            } catch (tcp_conn_t::write_closed_exc_t &e) {
                we_are_syncing = false;
                return -1;
            }
        }

    private:
        DISABLE_COPYING(tcp_conn_streambuf_out_t);

        tcp_conn_t *conn;

        // Write buffer
        // TODO: Do we have to tune this?
        static const size_t PUT_BUF_LENGTH = 512;
        char put_buf[PUT_BUF_LENGTH];

        bool we_are_syncing;
    };

    friend class streamed_tcp_listener_t;
    // Used by streamed_tcp_listener_t
    explicit streamed_tcp_conn_t(boost::scoped_ptr<tcp_conn_t> &conn) :
            conn_streambuf_in(conn.get()),
            conn_streambuf_out(conn.get()),
            istream(&conn_streambuf_in),
            ostream(&conn_streambuf_out) {
        conn_.swap(conn);
    }

    boost::scoped_ptr<tcp_conn_t> conn_;
    tcp_conn_streambuf_in_t conn_streambuf_in;
    tcp_conn_streambuf_out_t conn_streambuf_out;
    std::istream istream;
    std::ostream ostream;
};

/*
 tcp_listener_t creates tcp_conn_t objects. We want to be able to get
 streamed_tcp_conn_ts out of it. The following class provides a safe
 wrapper around tcp_listener_t which does exactly that.
*/

class streamed_tcp_listener_t {
public:
    streamed_tcp_listener_t(int port, boost::function<void(boost::scoped_ptr<streamed_tcp_conn_t>&)> _callback)
	: callback(_callback), listener(port, boost::bind(&streamed_tcp_listener_t::handle, this, _1) ) { }

private:
    // The wrapping handler
    void handle(boost::scoped_ptr<nascent_tcp_conn_t> &nconn) {
        boost::scoped_ptr<tcp_conn_t> conn;
        nconn->ennervate(conn);
        boost::scoped_ptr<streamed_tcp_conn_t> streamed_conn(new streamed_tcp_conn_t(conn));
        callback(streamed_conn);
    }

    // The callback to call when we get a connection
    boost::function<void(boost::scoped_ptr<streamed_tcp_conn_t>&)> callback;

    tcp_listener_t listener;
};

#endif
