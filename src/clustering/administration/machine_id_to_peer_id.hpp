#ifndef CLUSTERING_ADMINISTRATION_MACHINE_ID_TO_PEER_ID_HPP_
#define CLUSTERING_ADMINISTRATION_MACHINE_ID_TO_PEER_ID_HPP_

#include <map>

#include "clustering/administration/machine_metadata.hpp"

inline peer_id_t machine_id_to_peer_id(const machine_id_t &input, const std::map<peer_id_t, machine_id_t> &translation_table) {
    for (std::map<peer_id_t, machine_id_t>::const_iterator it = translation_table.begin(); it != translation_table.end(); it++) {
        if (it->second == input) {
            return it->first;
        }
    }
    return peer_id_t();
}

#endif /* CLUSTERING_ADMINISTRATION_MACHINE_ID_TO_PEER_ID_HPP_ */
