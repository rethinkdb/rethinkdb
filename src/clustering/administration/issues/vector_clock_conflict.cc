// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/issues/vector_clock_conflict.hpp"

namespace {

template<class type_t>
void check(const std::string &object_type, const uuid_u &object_id,
        const std::string &field, const vclock_t<type_t> &vector_clock,
        std::list<clone_ptr_t<vector_clock_conflict_issue_t> > *out) {

    if (vector_clock.in_conflict()) {
        out->push_back(clone_ptr_t<vector_clock_conflict_issue_t>(
            new vector_clock_conflict_issue_t(object_type, object_id, field)));
    }
}

void check_namespaces_for_protocol(
        const cow_ptr_t<namespaces_semilattice_metadata_t> &namespaces,
        std::list<clone_ptr_t<vector_clock_conflict_issue_t> > *out) {

    for (namespaces_semilattice_metadata_t::namespace_map_t::const_iterator it =
            namespaces->namespaces.begin(); it != namespaces->namespaces.end(); it++) {
        if (!it->second.is_deleted()) {
            check("namespace", it->first, "blueprint", it->second.get_ref().blueprint, out);
            check("namespace", it->first, "primary_uuid", it->second.get_ref().primary_datacenter, out);
            check("namespace", it->first, "replica_affinities", it->second.get_ref().replica_affinities, out);
            check("namespace", it->first, "ack_expectations", it->second.get_ref().ack_expectations, out);
            check("namespace", it->first, "shards", it->second.get_ref().shards, out);
            check("namespace", it->first, "name", it->second.get_ref().name, out);
            check("namespace", it->first, "primary_pinnings", it->second.get_ref().primary_pinnings, out);
            check("namespace", it->first, "secondary_pinnings", it->second.get_ref().secondary_pinnings, out);
            check("namespace", it->first, "database", it->second.get_ref().database, out);
        }
    }
}

}   /* anonymous namespace */

vector_clock_conflict_issue_tracker_t::vector_clock_conflict_issue_tracker_t(
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _cluster_view,
    boost::shared_ptr<semilattice_read_view_t<auth_semilattice_metadata_t> > _auth_view) :
    cluster_view(_cluster_view),
    auth_view(_auth_view) { }

vector_clock_conflict_issue_tracker_t::~vector_clock_conflict_issue_tracker_t() { }

std::list<clone_ptr_t<vector_clock_conflict_issue_t> > vector_clock_conflict_issue_tracker_t::get_vector_clock_issues() {
    cluster_semilattice_metadata_t cluster_metadata = cluster_view->get();
    auth_semilattice_metadata_t auth_metadata = auth_view->get();

    std::list<clone_ptr_t<vector_clock_conflict_issue_t> > issues;

    // Check cluster metadata
    check_namespaces_for_protocol(cluster_metadata.rdb_namespaces, &issues);

    for (datacenters_semilattice_metadata_t::datacenter_map_t::const_iterator it =
            cluster_metadata.datacenters.datacenters.begin();
            it != cluster_metadata.datacenters.datacenters.end(); it++) {
        if (!it->second.is_deleted()) {
            check("datacenter", it->first, "name", it->second.get_ref().name, &issues);
        }
    }

    for (databases_semilattice_metadata_t::database_map_t::const_iterator it =
            cluster_metadata.databases.databases.begin();
            it != cluster_metadata.databases.databases.end(); it++) {
        if (!it->second.is_deleted()) {
            check("database", it->first, "name", it->second.get_ref().name, &issues);
        }
    }

    for (machines_semilattice_metadata_t::machine_map_t::const_iterator it =
            cluster_metadata.machines.machines.begin();
            it != cluster_metadata.machines.machines.end(); it++) {
        if (!it->second.is_deleted()) {
            check("machine", it->first, "datacenter_uuid", it->second.get_ref().datacenter, &issues);
            check("machine", it->first, "name", it->second.get_ref().name, &issues);
        }
    }

    // Check auth metadata
    check("auth_key", nil_uuid(), "auth_key", auth_metadata.auth_key, &issues);

    return issues;
}

std::list<clone_ptr_t<global_issue_t> > vector_clock_conflict_issue_tracker_t::get_issues() {
    std::list<clone_ptr_t<vector_clock_conflict_issue_t> > vector_clock_issues = get_vector_clock_issues();
    std::list<clone_ptr_t<global_issue_t> > global_issues;

    for (std::list<clone_ptr_t<vector_clock_conflict_issue_t> >::iterator i = vector_clock_issues.begin();
         i != vector_clock_issues.end(); ++i)
        global_issues.push_back(clone_ptr_t<global_issue_t>((*i)->clone()));

    return global_issues;
}
