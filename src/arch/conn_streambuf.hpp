#ifndef __ARCH_CONN_STREAMBUF_HPP__
#define __ARCH_CONN_STREAMBUF_HPP__

/* This file contains an std::streambuf implementation that works 
with a tcp_conn_t object. This allows us to create std::ostream and std::istream
objects that interact directly with a tcp_conn_t TCP connection. For example
boost serialize requires streams for serializing data. 
This implementation does read buffering itself (where the buffering logic is
inherited from basic_streambuf). This is mostly because linux_tcp_conn_t::pop
is badly implemented and basic_streambuf has that logic implemented anyway.
Eventually we should rather fix buffering in the tcp_conn_t implementation and
make this synchronous. */

#include <streambuf>
#include "arch/arch.hpp"

/*
Usage example:
    tcp_conn_t conn(...);
    tcp_conn_streambuf_t streambuf(&conn);
    std::iostream stream(&streambuf);
    stream << "Hi, what's your name?" << std::endl;
    std::string name;
    stream >> name;
    stream << "Welcome " << name << "!" << std::endl;
    stream << "How old are you?" << std::endl;
    int age;
    stream >> age;
    stream << age << " is a nice age." << std::endl;
*/

// TODO! This is just severely destructed right now, just to allow sharing the conn with protobuffers...
// TODO! Make protobuffers use the stream! (and change the interface, so it's either a stream conn or a legacy conn)

class tcp_conn_streambuf_t : public std::basic_streambuf<char, std::char_traits<char> > {
public:
    tcp_conn_streambuf_t(tcp_conn_t *conn) : conn(conn) {
        // Initialize basic_streambuf, with gets being buffered and puts being unbuffered
        setg(&get_buf[0], &get_buf[0], &get_buf[0]);
        setp(0, 0);
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
                fprintf(stderr, "G: EOF\n");
                return std::char_traits<char>::eof();
            }
        }
        
        int i = std::char_traits<char>::to_int_type(*gptr());
        fprintf(stderr, "G: %c\n", (char)i);
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
                // TODO!
                //conn->write_buffered(&c, 1);
                conn->write(&c, 1);
                fprintf(stderr, "S: %c\n", c);
            } catch (tcp_conn_t::write_closed_exc_t &e) {
                return std::char_traits<char>::eof();
            }
            return std::char_traits<char>::not_eof(i);
        }
    }
    
    virtual int sync() {
        try {
            // TODO!
            //conn->flush_buffer();
            return 0;
        } catch (tcp_conn_t::write_closed_exc_t &e) {
            return -1;
        }
    }
    
private:
    // Read buffer
    // TODO!
    static const size_t GET_BUF_LENGTH = 1;
    //static const size_t GET_BUF_LENGTH = 512;
    char get_buf[GET_BUF_LENGTH];
    
    DISABLE_COPYING(tcp_conn_streambuf_t);
};

struct streamed_tcp_conn_t{

    streamed_tcp_conn_t(tcp_conn_t *c) :
        conn(c), streambuf(conn.get()), stream(&streambuf) { }

    boost::scoped_ptr<tcp_conn_t> conn;
    std::streambuf streambuf;
    std::iostream stream;
};

#endif
