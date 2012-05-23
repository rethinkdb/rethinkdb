#ifndef CLUSTERING_ADMINISTRATION_SUGGESTER_TCC_
#define CLUSTERING_ADMINISTRATION_SUGGESTER_TCC_

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/administration/suggester.hpp"

template<class protocol_t>
persistable_blueprint_t<protocol_t> suggest_blueprint_for_namespace(
        const namespace_semilattice_metadata_t<protocol_t> &ns_goals,
        const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
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
        typename std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > >::const_iterator jt =
            reactor_directory_view.find(peer);
        if (jt != reactor_directory_view.end() && jt->second) {
            directory.insert(std::make_pair(machine, jt->second->internal));
        }
    }

    datacenter_id_t primary_datacenter =
        ns_goals.primary_datacenter.get();

    std::map<datacenter_id_t, int> datacenter_affinities =
        ns_goals.replica_affinities.get();

    std::set<typename protocol_t::region_t> shards =
        ns_goals.shards.get();

    region_map_t<protocol_t, machine_id_t> primary_pinnings =
        ns_goals.primary_pinnings.get();

    region_map_t<protocol_t, std::set<machine_id_t> > secondary_pinnings =
        ns_goals.secondary_pinnings.get();

    return suggest_blueprint(directory, primary_datacenter,
        datacenter_affinities, shards, machine_data_centers,
        primary_pinnings, secondary_pinnings);
}

template<class protocol_t>
std::map<namespace_id_t, persistable_blueprint_t<protocol_t> > suggest_blueprints_for_protocol(
        const namespaces_semilattice_metadata_t<protocol_t> &ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > &namespaces_directory,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers)
        THROWS_ONLY(missing_machine_exc_t)
{

    std::map<namespace_id_t, persistable_blueprint_t<protocol_t> > out;
    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::const_iterator it  = ns_goals.namespaces.begin();
                                                                                                 it != ns_goals.namespaces.end();
                                                                                                 ++it) {
        if (!it->second.is_deleted()) {
            std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > reactor_directory;
            for (typename std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> >::const_iterator jt =
                    namespaces_directory.begin(); jt != namespaces_directory.end(); jt++) {
                typename std::map<namespace_id_t, directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >::const_iterator kt =
                    jt->second.reactor_bcards.find(it->first);
                if (kt != jt->second.reactor_bcards.end()) {
                    reactor_directory.insert(std::make_pair(
                        jt->first,
                        boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >(kt->second)
                        ));
                } else {
                    reactor_directory.insert(std::make_pair(
                        jt->first,
                        boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > >()
                        ));
                }
            }
            try {
                out.insert(
                    std::make_pair(it->first,
                        suggest_blueprint_for_namespace<protocol_t>(
                            it->second.get(),
                            reactor_directory,
                            machine_id_translation_table,
                            machine_data_centers
                            )));
            } catch (cannot_satisfy_goals_exc_t &e) {
                logERR("Namespace %s has unsatisfiable goals", uuid_to_str(it->first).c_str());
            } catch (in_conflict_exc_t &e) {
                logERR("Namespace %s has internal conflicts", uuid_to_str(it->first).c_str());
            }
        }
    }
    return out;
}

template<class protocol_t>
void fill_in_blueprints_for_protocol(
        namespaces_semilattice_metadata_t<protocol_t> *ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > &namespaces_directory,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const machine_id_t &us)
        THROWS_ONLY(missing_machine_exc_t)
{
    typedef std::map<namespace_id_t, persistable_blueprint_t<protocol_t> > blueprint_map_t;
    blueprint_map_t suggested_blueprints =
        suggest_blueprints_for_protocol(*ns_goals, namespaces_directory, machine_id_translation_table, machine_data_centers);

    for (typename blueprint_map_t::iterator it  = suggested_blueprints.begin();
                                            it != suggested_blueprints.end();
                                            ++it) {
        ns_goals->namespaces[it->first].get_mutable().blueprint = ns_goals->namespaces[it->first].get().blueprint.make_new_version(it->second, us);
    }
}

#endif  // CLUSTERING_ADMINISTRATION_SUGGESTER_TCC_
