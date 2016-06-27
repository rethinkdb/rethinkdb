// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "client_protocol/protocols.hpp"

#include <limits>

const uint32_t wire_protocol_t::HARD_LIMIT_TOO_LARGE_QUERY_SIZE = GIGABYTE;
const uint32_t wire_protocol_t::TOO_LONG_QUERY_TIME = 5*60*1000; // ms
const uint32_t wire_protocol_t::TOO_LARGE_QUERY_SIZE = 128 * MEGABYTE;
const uint32_t wire_protocol_t::TOO_LARGE_RESPONSE_SIZE =
    std::numeric_limits<uint32_t>::max();

const std::string wire_protocol_t::unparseable_query_message =
    "Client is buggy (failed to deserialize query).";

std::string wire_protocol_t::too_large_query_message(uint32_t size) {
    return strprintf("Query size (%" PRIu32 ") greater than maximum (%" PRIu32 ").",
                     size, TOO_LARGE_QUERY_SIZE - 1);
}

std::string wire_protocol_t::too_large_response_message(size_t size) {
    return strprintf("Response size (%zu) greater than maximum (%" PRIu32 ").",
                     size, TOO_LARGE_RESPONSE_SIZE - 1);
}
