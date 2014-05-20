// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rpc/connectivity/messages.hpp"

#include <functional>

#include "arch/runtime/coroutines.hpp"
#include "containers/archive/vector_stream.hpp"
#include "rpc/connectivity/cluster.hpp"


void message_handler_t::on_local_message(peer_id_t source_peer,
                                         cluster_version_t cluster_version,
                                         std::vector<char> &&data) {
    // This is only sensible.  We pass the cluster_version all the way from the local
    // serialization code just to play nice.
    rassert(cluster_version == cluster_version_t::LATEST_VERSION);

    vector_read_stream_t read_stream(std::move(data));
    on_message(source_peer, cluster_version, &read_stream);
}
