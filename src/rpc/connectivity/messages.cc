// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rpc/connectivity/messages.hpp"

#include <functional>

#include "arch/runtime/coroutines.hpp"
#include "containers/archive/vector_stream.hpp"
#include "rpc/connectivity/cluster.hpp"


void message_handler_t::on_local_message(peer_id_t source_peer,
                                         std::vector<char> &&data) {
    vector_read_stream_t read_stream(std::move(data));
    on_message(source_peer, &read_stream);
}
