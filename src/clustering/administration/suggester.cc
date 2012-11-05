// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/suggester.hpp"
#include <deque>

#include "clustering/administration/machine_id_to_peer_id.hpp"

template<class protocol_t>
persistable_blueprint_t<protocol_t> suggest_blueprint_for_namespace(
        const namespace_semilattice_metadata_t<protocol_t> &ns_goals,
        const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t) {

    std::map<machine_id_t, reactor_business_card_t<protocol_t> > directory;
    for (std::map<machine_id_t, datacenter_id_t>::const_iterator it = machine_data_centers.begin();
            it != machine_data_centers.end(); it++) {
        machine_id_t machine = it->first;
        peer_id_t peer = machine_id_to_peer_id(machine, machine_id_translation_table);
        if (peer.is_nil()) {
            throw missing_machine_exc_t();
        }
        typename std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > >::const_iterator jt =
            reactor_directory_view.find(peer);
        if (jt != reactor_directory_view.end() && jt->second) {
            directory.insert(std::make_pair(machine, *jt->second->internal));
        }
    }

    datacenter_id_t primary_datacenter =
        ns_goals.primary_datacenter.get();

    std::map<datacenter_id_t, int> datacenter_affinities =
        ns_goals.replica_affinities.get();

    nonoverlapping_regions_t<protocol_t> shards =
        ns_goals.shards.get();

    region_map_t<protocol_t, machine_id_t> primary_pinnings =
        ns_goals.primary_pinnings.get();

    region_map_t<protocol_t, std::set<machine_id_t> > secondary_pinnings =
        ns_goals.secondary_pinnings.get();

    return suggest_blueprint(directory, primary_datacenter,
        datacenter_affinities, shards, machine_data_centers,
        primary_pinnings, secondary_pinnings, usage, prioritize_distribution);
}

template<class protocol_t>
std::map<namespace_id_t, persistable_blueprint_t<protocol_t> > suggest_blueprints_for_protocol(
        const namespaces_semilattice_metadata_t<protocol_t> &ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> > &namespaces_directory,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        bool prioritize_distribution)
        THROWS_ONLY(missing_machine_exc_t)
{
    std::map<machine_id_t, int> usage;
    std::map<namespace_id_t, persistable_blueprint_t<protocol_t> > out;

    /* The suggester can give much better results if it first does the
     * namespaces which have already had blueprints drawn and then does the new
     * blueprints. This is because namespaces which previously had blueprints
     * tend to have more constraints than new ones. This is a bit of a hack to
     * acheive this since it would be much better to have and algorithm that
     * understands how much constraint each table has a does the most
     * constrained first. However it seems like this actually boils down to a
     * graph coloring problem so it's probably NP-Complete. */
    std::deque<namespace_id_t> order;
    for (typename namespaces_semilattice_metadata_t<protocol_t>::namespace_map_t::const_iterator it  = ns_goals.namespaces.begin();
                                                                                                 it != ns_goals.namespaces.end();
                                                                                                 ++it) {
        if (!it->second.is_deleted()) {
            if (it->second.get().blueprint.in_conflict() || !(it->second.get().blueprint.get() == persistable_blueprint_t<protocol_t>())) {
                order.push_front(it->first);
            } else {
                order.push_back(it->first);
            }
        }
    }

    for (std::deque<namespace_id_t>::iterator it  = order.begin();
                                              it != order.end();
                                              ++it) {
        std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > > > reactor_directory;
        for (typename std::map<peer_id_t, namespaces_directory_metadata_t<protocol_t> >::const_iterator jt =
                namespaces_directory.begin(); jt != namespaces_directory.end(); jt++) {
            typename std::map<namespace_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >::const_iterator kt =
                jt->second.reactor_bcards.find(*it);
            if (kt != jt->second.reactor_bcards.end()) {
                reactor_directory.insert(std::make_pair(
                    jt->first,
                    boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >(kt->second)));
            } else {
                reactor_directory.insert(std::make_pair(
                    jt->first,
                    boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<protocol_t> > > >()));
            }
        }
        try {
            out.insert(
                std::make_pair(*it,
                    suggest_blueprint_for_namespace<protocol_t>(
                        ns_goals.namespaces.find(*it)->second.get(),
                        reactor_directory,
                        machine_id_translation_table,
                        machine_data_centers,
                        &usage,
                        prioritize_distribution)));
        } catch (const cannot_satisfy_goals_exc_t &e) {
            logERR("Namespace %s has unsatisfiable goals", uuid_to_str(*it).c_str());
        } catch (const in_conflict_exc_t &e) {
            logERR("Namespace %s has internal conflicts", uuid_to_str(*it).c_str());
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
        const machine_id_t &us,
        bool prioritize_distribution)
        THROWS_ONLY(missing_machine_exc_t)
{
    typedef std::map<namespace_id_t, persistable_blueprint_t<protocol_t> > blueprint_map_t;
    blueprint_map_t suggested_blueprints =
        suggest_blueprints_for_protocol(*ns_goals, namespaces_directory, machine_id_translation_table, machine_data_centers, prioritize_distribution);

    for (typename blueprint_map_t::iterator it  = suggested_blueprints.begin();
                                            it != suggested_blueprints.end();
                                            ++it) {
        ns_goals->namespaces[it->first].get_mutable()->blueprint = ns_goals->namespaces[it->first].get().blueprint.make_resolving_version(it->second, us);
    }
}


void fill_in_blueprints(cluster_semilattice_metadata_t *cluster_metadata,
                        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
                        const uuid_t &us,
                        bool prioritize_distribution) {
    std::map<machine_id_t, datacenter_id_t> machine_assignments;

    for (std::map<machine_id_t, deletable_t<machine_semilattice_metadata_t> >::iterator it = cluster_metadata->machines.machines.begin();
            it != cluster_metadata->machines.machines.end();
            it++) {
        if (!it->second.is_deleted()) {
            if (!it->second.get().datacenter.in_conflict()) {
                machine_assignments[it->first] = it->second.get().datacenter.get();
            }
        }
    }

    std::map<peer_id_t, namespaces_directory_metadata_t<memcached_protocol_t> > reactor_directory_memcached;
    std::map<peer_id_t, namespaces_directory_metadata_t<rdb_protocol_t> > reactor_directory_rdb;
    std::map<peer_id_t, machine_id_t> machine_id_translation_table;
    for (std::map<peer_id_t, cluster_directory_metadata_t>::const_iterator it = directory.begin(); it != directory.end(); it++) {
        reactor_directory_memcached.insert(std::make_pair(it->first, it->second.memcached_namespaces));
        reactor_directory_rdb.insert(std::make_pair(it->first, it->second.rdb_namespaces));
        if (it->second.peer_type == SERVER_PEER) {
            machine_id_translation_table.insert(std::make_pair(it->first, it->second.machine_id));
        }
    }

    {
        cow_ptr_t<namespaces_semilattice_metadata_t<memcached_protocol_t> >::change_t change(&cluster_metadata->memcached_namespaces);
        fill_in_blueprints_for_protocol<memcached_protocol_t>(change.get(),
                reactor_directory_memcached,
                machine_id_translation_table,
                machine_assignments,
                us,
                prioritize_distribution);
    }

    {
        cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&cluster_metadata->rdb_namespaces);
        fill_in_blueprints_for_protocol<rdb_protocol_t>(change.get(),
                reactor_directory_rdb,
                machine_id_translation_table,
                machine_assignments,
                us,
                prioritize_distribution);
    }
}


#include "mock/dummy_protocol.hpp"

template
persistable_blueprint_t<mock::dummy_protocol_t> suggest_blueprint_for_namespace<mock::dummy_protocol_t>(
        const namespace_semilattice_metadata_t<mock::dummy_protocol_t> &ns_goals,
        const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<mock::dummy_protocol_t> > > > > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t);

template
std::map<namespace_id_t, persistable_blueprint_t<mock::dummy_protocol_t> > suggest_blueprints_for_protocol<mock::dummy_protocol_t>(
        const namespaces_semilattice_metadata_t<mock::dummy_protocol_t> &ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t<mock::dummy_protocol_t> > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        bool prioritize_distribution)
        THROWS_ONLY(missing_machine_exc_t);

template
void fill_in_blueprints_for_protocol<mock::dummy_protocol_t>(
        namespaces_semilattice_metadata_t<mock::dummy_protocol_t> *ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t<mock::dummy_protocol_t> > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const machine_id_t &us,
        bool prioritize_disribution)
        THROWS_ONLY(missing_machine_exc_t);


#include "memcached/protocol.hpp"

template
persistable_blueprint_t<memcached_protocol_t> suggest_blueprint_for_namespace<memcached_protocol_t>(
        const namespace_semilattice_metadata_t<memcached_protocol_t> &ns_goals,
        const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<memcached_protocol_t> > > > > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t);

template
std::map<namespace_id_t, persistable_blueprint_t<memcached_protocol_t> > suggest_blueprints_for_protocol<memcached_protocol_t>(
        const namespaces_semilattice_metadata_t<memcached_protocol_t> &ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t<memcached_protocol_t> > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        bool prioritize_distribution)
        THROWS_ONLY(missing_machine_exc_t);

template
void fill_in_blueprints_for_protocol<memcached_protocol_t>(
        namespaces_semilattice_metadata_t<memcached_protocol_t> *ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t<memcached_protocol_t> > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const machine_id_t &us,
        bool prioritize_disribution)
        THROWS_ONLY(missing_machine_exc_t);

#include "rdb_protocol/protocol.hpp"

template
persistable_blueprint_t<rdb_protocol_t> suggest_blueprint_for_namespace<rdb_protocol_t>(
        const namespace_semilattice_metadata_t<rdb_protocol_t> &ns_goals,
        const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t<rdb_protocol_t> > > > > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t);

template
std::map<namespace_id_t, persistable_blueprint_t<rdb_protocol_t> > suggest_blueprints_for_protocol<rdb_protocol_t>(
        const namespaces_semilattice_metadata_t<rdb_protocol_t> &ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t<rdb_protocol_t> > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        bool prioritize_distribution)
        THROWS_ONLY(missing_machine_exc_t);

template
void fill_in_blueprints_for_protocol<rdb_protocol_t>(
        namespaces_semilattice_metadata_t<rdb_protocol_t> *ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t<rdb_protocol_t> > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const machine_id_t &us,
        bool prioritize_disribution)
        THROWS_ONLY(missing_machine_exc_t);
