// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_SERVER_ID_TO_PEER_ID_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_SERVER_ID_TO_PEER_ID_HPP_

#include <map>
#include <utility>

#include "clustering/administration/servers/server_metadata.hpp"
#include "rpc/connectivity/peer_id.hpp"

inline peer_id_t server_id_to_peer_id(const server_id_t &input, const std::map<peer_id_t, server_id_t> &translation_table) {
    for (std::map<peer_id_t, server_id_t>::const_iterator it = translation_table.begin(); it != translation_table.end(); ++it) {
        if (it->second == input) {
            return it->first;
        }
    }
    return peer_id_t();
}

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_SERVER_ID_TO_PEER_ID_HPP_ */
