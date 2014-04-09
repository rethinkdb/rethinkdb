// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SUGGESTER_HPP_
#define CLUSTERING_ADMINISTRATION_SUGGESTER_HPP_

#include <map>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "clustering/suggester/suggest_blueprint.hpp"
#include "clustering/administration/metadata.hpp"

struct missing_machine_exc_t : public std::exception {
    const char *what() const throw () {
        return "Cannot compute blueprint because a machine is offline.";
    }
};

persistable_blueprint_t suggest_blueprint_for_namespace(
        const namespace_semilattice_metadata_t &ns_goals,
        const std::map<peer_id_t, boost::optional<directory_echo_wrapper_t<cow_ptr_t<reactor_business_card_t> > > > &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        bool prioritize_distribution)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t);

std::map<namespace_id_t, persistable_blueprint_t> suggest_blueprints_for_protocol(
        const namespaces_semilattice_metadata_t &ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t> &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const boost::optional<namespace_id_t> &prioritize_distr_for_ns)
        THROWS_ONLY(missing_machine_exc_t);

void fill_in_blueprints_for_protocol(
        namespaces_semilattice_metadata_t *ns_goals,
        const std::map<peer_id_t, namespaces_directory_metadata_t> &reactor_directory_view,
        const std::map<peer_id_t, machine_id_t> &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers,
        const machine_id_t &us,
        const boost::optional<namespace_id_t> &prioritize_distr_for_ns)
        THROWS_ONLY(missing_machine_exc_t);

void fill_in_blueprints(cluster_semilattice_metadata_t *cluster_metadata,
        const std::map<peer_id_t, cluster_directory_metadata_t> &directory,
        const uuid_u &us,
        const boost::optional<namespace_id_t> &prioritize_distr_for_ns);

#endif /* CLUSTERING_ADMINISTRATION_SUGGESTER_HPP_ */
