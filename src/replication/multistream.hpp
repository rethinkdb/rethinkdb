#ifndef __REPLICATION_MULTISTREAM_HPP__
#define __REPLICATION_MULTISTREAM_HPP__

#include "arch/arch.hpp"

// TODO: We include this for net_hello_t, and it is in the wrong place or this is.
#include "replication/net_structs.hpp"

// TODO: Right now we only have the code for reading from a stream
// here.  We haven't generified the code for writing to a stream.

namespace replication {
extern const uint32_t MAX_MESSAGE_SIZE;

// TODO: Do we ever really handle these?

// TODO: We should have separate exception types for multistream
// exceptions and replication protocol exceptions.
class protocol_exc_t : public std::exception {
public:
    protocol_exc_t(const char *msg) : msg_(msg) { }
    const char *what() const throw() { return msg_; }
private:
    const char *msg_;
};

struct stream_handler_t {
    virtual void stream_part(const char *buf, size_t size) = 0;
    virtual void end_of_stream() = 0;

    virtual ~stream_handler_t() { }
};

struct connection_handler_t {
    // TODO: Do something about the use of net_hello_t here.
    virtual void process_hello_message(net_hello_t msg) = 0;
    virtual stream_handler_t *new_stream_handler() = 0;
    virtual void conn_closed() = 0;
protected:
    virtual ~connection_handler_t() { }
};

void parse_messages(tcp_conn_t *conn, connection_handler_t *receiver);





}  // namespace replication


#endif  // __REPLICATION_MULTISTREAM_HPP__
