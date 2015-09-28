// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLIENT_PROTOCOL_PROTOCOLS_HPP_
#define CLIENT_PROTOCOL_PROTOCOLS_HPP_

#include <stdint.h>

#include <string>

// Include all available wire protocols
#include "client_protocol/json.hpp"

// Contains common declarations used by all wire protocols, this is a class rather than
// a namespace so we don't have to extern stuff.
class wire_protocol_t {
public:
    static const uint32_t TOO_LARGE_QUERY_SIZE;
    static const uint32_t TOO_LARGE_RESPONSE_SIZE;

    static const std::string unparseable_query_message;
    static std::string too_large_query_message(uint32_t size);
    static std::string too_large_response_message(size_t size);
};

#endif // CLIENT_PROTOCOL_PROTOCOLS_HPP_
