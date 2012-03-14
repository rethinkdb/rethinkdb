#ifndef __CLUSTERING_ADMINISTRATION_SUGGESTER_HPP__
#define __CLUSTERING_ADMINISTRATION_SUGGESTER_HPP__

#include "errors.hpp"

#include "clustering/suggester/suggester.hpp"

struct missing_machine_exc_t : public std::exception {
    const char *what() const throw ();
};

template<class protocol_t>
persistable_blueprint_t<protocol_t> suggest_blueprint_for_namespace(
        const namespace_semilattice_metadata_t<protocol_t> &ns_goals,
        const clone_ptr_t<directory_rview_t<boost::optional<directory_echo_wrapper_t<reactor_business_card_t<protocol_t> > > > > &reactor_directory_view,
        const clone_ptr_t<directory_rview_t<machine_id_t> > &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t);

template<class protocol_t>
std::map<namespace_id_t, persistable_blueprint_t<protocol_t> > suggest_blueprints_for_protocol(
        const namespaces_semilattice_metadata_t<protocol_t> &ns_goals,
        const clone_ptr_t<directory_rview_t<namespaces_directory_metadata_t<protocol_t> > > &reactor_directory_view,
        const clone_ptr_t<directory_rview_t<machine_id_t> > &machine_id_translation_table,
        const std::map<machine_id_t, datacenter_id_t> &machine_data_centers)
        THROWS_ONLY(cannot_satisfy_goals_exc_t, in_conflict_exc_t, missing_machine_exc_t);

#include "clustering/administration/suggester.tcc"
#endif
