#ifndef CONTAINERS_ARCHIVE_ORDER_TOKEN_HPP_
#define CONTAINERS_ARCHIVE_ORDER_TOKEN_HPP_

#include "containers/archive/archive.hpp"

#include "concurrency/fifo_checker.hpp"

// Apparently, this is what we were doing before.  We just send/receive ignore.
inline MUST_USE int deserialize(UNUSED read_stream_t *s, order_token_t *tok) {
    *tok = order_token_t::ignore;
    return ARCHIVE_SUCCESS;
}

// Send no evil, receive no evil.
inline write_message_t &operator<<(write_message_t &msg, UNUSED const order_token_t &tok) {
    return msg;
}



#endif  // CONTAINERS_ARCHIVE_ORDER_TOKEN_HPP_
