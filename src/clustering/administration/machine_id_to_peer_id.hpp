#ifndef __CLUSTERING_ADMINISTRATION_MACHINE_ID_TO_PEER_ID_HPP__
#define __CLUSTERING_ADMINISTRATION_MACHINE_ID_TO_PEER_ID_HPP__

inline peer_id_t machine_id_to_peer_id(const machine_id_t &input, const clone_ptr_t<directory_rview_t<machine_id_t> > &translation_table) {
    std::set<peer_id_t> peers_list = translation_table->get_directory_service()->get_connectivity_service()->get_peers_list();
    for (std::set<peer_id_t>::iterator it = peers_list.begin(); it != peers_list.end(); it++) {
        machine_id_t machine_id = translation_table->get_value(*it).get();
        if (machine_id == input) {
            return *it;
        }
    }
    return peer_id_t();
}

#endif /* __CLUSTERING_ADMINISTRATION_MACHINE_ID_TO_PEER_ID_HPP__ */
