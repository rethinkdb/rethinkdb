// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "clustering/administration/suggester.hpp"

#include <deque>
#include <utility>

#include "clustering/administration/machine_id_to_peer_id.hpp"
#include "clustering/reactor/directory_echo.hpp"
#include "containers/cow_ptr.hpp"

persistable_blueprint_t suggest_blueprint_for_namespace(
        namespace_id_t namespace_id,
        const namespace_semilattice_metadata_t &ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t> &namespaces_directory,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        std::map<machine_id_t, int> *usage,
        bool prioritize_distribution)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t) {

    std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > > reactor_directory;
    for (std::map<peer_id_t, namespaces_directory_metadata_t>::const_iterator jt =
            namespaces_directory.begin(); jt != namespaces_directory.end(); jt++) {
        std::map<namespace_id_t, directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >::const_iterator kt =
            jt->second.reactor_bcards.find(namespace_id);
        if (kt != jt->second.reactor_bcards.end()) {
            reactor_directory.insert(std::make_pair(
                jt->first,
                boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >(kt->second)));
        } else {
            reactor_directory.insert(std::make_pair(
                jt->first,
                boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > >()));
        }
    }

    std::map<machine_id_t, reactor_business_card_t> directory;
    for (std::map<machine_id_t, datacenter_id_t>::const_iterator it = machine_data_centers.begin();
            it != machine_data_centers.end(); it++) {
        machine_id_t machine = it->first;
        peer_id_t peer = machine_id_to_peer_id(machine, machine_id_translation_table);
        if (peer.is_nil()) {
            throw missing_machine_exc_t();
        }
        std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > >::const_iterator jt =
            reactor_directory.find(peer);
        if (jt != reactor_directory.end() && jt->second) {
            directory.insert(std::make_pair(machine, *jt->second->internal));
        }
    }

    datacenter_id_t primary_datacenter =
        ns_goals.primary_datacenter.get();

    std::map<datacenter_id_t, int32_t> datacenter_affinities =
        ns_goals.replica_affinities.get();

    nonoverlapping_regions_t shards =
        ns_goals.shards.get();

    region_map_t<machine_id_t> primary_pinnings =
        ns_goals.primary_pinnings.get();

    region_map_t<std::set<machine_id_t> > secondary_pinnings =
        ns_goals.secondary_pinnings.get();

    return suggest_blueprint(directory, primary_datacenter,
        datacenter_affinities, shards, machine_data_centers,
        primary_pinnings, secondary_pinnings, usage, prioritize_distribution);
}

std::map<namespace_id_t, persistable_blueprint_t> suggest_blueprints_for_protocol(
        const namespaces_semilattice_metadata_t &ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t> &namespaces_directory,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const boost::optional<namespace_id_t> &prioritize_distr_for_ns)
        THROWS_ONLY(missing_machine_exc_t)
{
    std::map<machine_id_t, int> usage;
    std::map<namespace_id_t, persistable_blueprint_t> out;

    /* The suggester can give much better results if it first does the
     * namespaces which have already had blueprints drawn and then does the new
     * blueprints. This is because namespaces which previously had blueprints
     * tend to have more constraints than new ones. This is a bit of a hack to
     * acheive this since it would be much better to have and algorithm that
     * understands how much constraint each table has a does the most
     * constrained first. However it seems like this actually boils down to a
     * graph coloring problem so it's probably NP-Complete. */
    std::deque<namespace_id_t> order;
    for (namespaces_semilattice_metadata_t::namespace_map_t::const_iterator it = ns_goals.namespaces.begin();
         it != ns_goals.namespaces.end();
         ++it) {
        if (!it->second.is_deleted()) {
            if (it->second.get_ref().blueprint.in_conflict() || !(it->second.get_ref().blueprint.get() == persistable_blueprint_t())) {
                order.push_front(it->first);
            } else {
                order.push_back(it->first);
            }
        }
    }

    for (std::deque<namespace_id_t>::iterator it  = order.begin();
                                              it != order.end();
                                              ++it) {
        try {
            const bool prioritize_distr = prioritize_distr_for_ns
                && prioritize_distr_for_ns.get() == *it;
            out.insert(
                std::make_pair(*it,
                    suggest_blueprint_for_namespace(
                        *it,
                        ns_goals.namespaces.find(*it)->second.get_copy(),
                        namespaces_directory,
                        machine_id_translation_table,
                        machine_data_centers,
                        &usage,
                        prioritize_distr)));
        } catch (const cannot_satisfy_goals_exc_t &e) {
            logERR("Namespace %s has unsatisfiable goals", uuid_to_str(*it).c_str());
        } catch (const in_conflict_exc_t &e) {
            logERR("Namespace %s has internal conflicts", uuid_to_str(*it).c_str());
        }
    }
    return out;
}

void fill_in_blueprints_for_protocol(
        namespaces_semilattice_metadata_t *ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t> &namespaces_directory,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const machine_id_t &us,
        const boost::optional<namespace_id_t> &prioritize_distr_for_ns)
        THROWS_ONLY(missing_machine_exc_t)
{
    typedef std::map<namespace_id_t, persistable_blueprint_t> blueprint_map_t;
    blueprint_map_t suggested_blueprints =
        suggest_blueprints_for_protocol(*ns_goals,
                                        namespaces_directory,
                                        machine_id_translation_table,
                                        machine_data_centers,
                                        prioritize_distr_for_ns);

    for (blueprint_map_t::iterator it  = suggested_blueprints.begin();
                                   it != suggested_blueprints.end();
                                   ++it) {
        ns_goals->namespaces[it->first].get_mutable()->blueprint =
            ns_goals->namespaces[it->first].get_copy().blueprint.make_resolving_version(it->second, us);
    }
}

static void make_caches(cluster_semilattice_metadata_t *cluster_metadata,
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        std::map<machine_id_t, datacenter_id_t> *machine_assignments_out,
        std::map<peer_id_t, namespaces_directory_metadata_t> *reactor_directory_rdb_out,
        std::map<peer_id_t, machine_id_t> *machine_id_translation_table_out) {
    for (auto it = cluster_metadata->machines.machines.begin();
              it != cluster_metadata->machines.machines.end();
            ++it) {
        if (!it->second.is_deleted()) {
            if (!it->second.get_ref().datacenter.in_conflict()) {
                (*machine_assignments_out)[it->first] =
                    it->second.get_ref().datacenter.get();
            } else {
                (*machine_assignments_out)[it->first] = nil_uuid();
            }
        }
    }
    for (auto it = directory.begin(); it != directory.end(); it++) {
        reactor_directory_rdb_out->insert(
            std::make_pair(it->first, it->second.rdb_namespaces));
        if (it->second.peer_type == SERVER_PEER) {
            machine_id_translation_table_out->insert(
                std::make_pair(it->first, it->second.machine_id));
        }
    }
}

void fill_in_all_blueprints(cluster_semilattice_metadata_t *cluster_metadata,
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        const uuid_u &us,
        const boost::optional<namespace_id_t> &prioritize_distr_for_ns) {
    std::map<machine_id_t, datacenter_id_t> machine_assignments;
    std::map<peer_id_t, namespaces_directory_metadata_t> reactor_directory_rdb;
    std::map<peer_id_t, machine_id_t> machine_id_translation_table;
    make_caches(cluster_metadata, directory,
        &machine_assignments, &reactor_directory_rdb, &machine_id_translation_table);

    {
        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t change(
            &cluster_metadata->rdb_namespaces);
        fill_in_blueprints_for_protocol(change.get(),
                reactor_directory_rdb,
                machine_id_translation_table,
                machine_assignments,
                us,
                prioritize_distr_for_ns);
    }
}

void fill_in_blueprint_for_namespace(cluster_semilattice_metadata_t *cluster_metadata,
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        namespace_id_t namespace_id,
        const uuid_u &us) {
    std::map<machine_id_t, datacenter_id_t> machine_assignments;
    std::map<peer_id_t, namespaces_directory_metadata_t> reactor_directory_rdb;
    std::map<peer_id_t, machine_id_t> machine_id_translation_table;
    make_caches(cluster_metadata, directory,
        &machine_assignments, &reactor_directory_rdb, &machine_id_translation_table);

    {
        cow_ptr_t<namespaces_semilattice_metadata_t>::change_t change(
            &cluster_metadata->rdb_namespaces);

        guarantee(change.get()->namespaces.count(namespace_id) == 1);
        if (change.get()->namespaces.at(namespace_id).is_deleted()) {
            return;
        }

        std::map<machine_id_t, int> usage;
        for (auto it = change.get()->namespaces.begin();
                  it != change.get()->namespaces.end();
                ++it) {
            if (it->first == namespace_id) {
                continue;
            }
            if (it->second.is_deleted()) {
                continue;
            }
            if (it->second.get_ref().blueprint.in_conflict()) {
                continue;
            }
            calculate_usage(it->second.get_ref().blueprint.get_ref(), &usage);
        }

        try {
            persistable_blueprint_t new_blueprint =
                suggest_blueprint_for_namespace(
                    namespace_id,
                    change.get()->namespaces.at(namespace_id).get_copy(),
                    reactor_directory_rdb,
                    machine_id_translation_table,
                    machine_assignments,
                    &usage,
                    false);
            change.get()->namespaces.at(namespace_id).get_mutable()->blueprint =
                change.get()->namespaces.at(namespace_id).get_copy().blueprint.
                    make_resolving_version(new_blueprint, us);
        } catch (const cannot_satisfy_goals_exc_t &e) {
            logERR("Namespace %s has unsatisfiable goals",
                uuid_to_str(namespace_id).c_str());
        } catch (const in_conflict_exc_t &e) {
            logERR("Namespace %s has internal conflicts",
                uuid_to_str(namespace_id).c_str());
        }
    }
}

