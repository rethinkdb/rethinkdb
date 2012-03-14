#ifndef __CLUSTERING_ADMINISTRATION_SUGGESTER_TCC__
#define __CLUSTERING_ADMINISTRATION_SUGGESTER_TCC__

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/administration/suggester.hpp"
#include "lens.hpp"

const char *missing_machine_exc_t::what() const throw () {
    return "Cannot compute blueprint because a machine is offline.";
}

template<class protocol_t>
persistable_blueprint_t<protocol_t> suggest_blueprint_for_namespace(
        const namespace_semilattice_metadata_t<protocol_t> &ns_goals,
        const clone_ptr_t<directory_rview_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > > &reactor_directory_view,
        const clone_ptr_t<directory_rview_t<machine_id_t> > &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t) {

    std::map<machine_id_t, reactor_business_card_t<protocol_t> > directory;
    for (std::map<machine_id_t, datacenter_id_t>::const_iterator it = machine_data_centers.begin();
            it != machine_data_centers.end(); it++) {
        machine_id_t machine = it->first;
        peer_id_t peer = machine_id_to_peer_id(machine, machine_id_translation_table);
        if (peer.is_nil()) {
            throw missing_machine_exc_t();
        }
        boost::optional<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > value =
            reactor_directory_view->get_value(peer);
        if (!value || !*value) {
            throw missing_machine_exc_t();
        }
        directory.insert(std::make_pair(machine, (**value).internal));
    }

    datacenter_id_t primary_datacenter =
        ns_goals.primary_datacenter.get();
    std::map<datacenter_id_t, int> datacenter_affinities =
        ns_goals.replica_affinities.get();
    std::set<typename protocol_t::region_t> shards =
        ns_goals.shards.get();

    return suggest_blueprint(directory, primary_datacenter,
        datacenter_affinities, shards, machine_data_centers);
}

template<class protocol_t>
std::map<namespace_id_t, persistable_blueprint_t<protocol_t> > suggest_blueprints_for_protocol(
        const namespaces_semilattice_metadata_t<protocol_t> &ns_goals,
        const clone_ptr_t<directory_rview_t<namespaces_directory_metadata_t<protocol_t> > > &reactor_directory_view,
        const clone_ptr_t<directory_rview_t<machine_id_t> > &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t) {

    std::map<namespace_id_t, persistable_blueprint_t<protocol_t> > out;
    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::const_iterator it  = ns_goals.namespaces.begin();
                                                                                                 it != ns_goals.namespaces.end(); 
                                                                                                 ++it) {
        if (!it->second.is_deleted()) {
            typedef std::map<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > bcard_map_t;
            out.insert(
                    std::make_pair(it->first,
                        suggest_blueprint_for_namespace<protocol_t>(
                            it->second.get(),
                            reactor_directory_view->subview(clone_ptr_t<read_lens_t<bcard_map_t, namespaces_directory_metadata_t<protocol_t> > >(field_lens(&namespaces_directory_metadata_t<protocol_t>::reactor_bcards)))
                            ->subview(clone_ptr_t<read_lens_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >, bcard_map_t> >(optional_member_lens<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >(it->first))),
                            machine_id_translation_table,
                            machine_data_centers
                            )));
        }
    }
    return out;
        }

#endif
