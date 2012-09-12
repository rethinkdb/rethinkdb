#ifndef CLUSTERING_ADMINISTRATION_ISSUES_PINNINGS_SHARDS_MISMATCH_HPP_
#define CLUSTERING_ADMINISTRATION_ISSUES_PINNINGS_SHARDS_MISMATCH_HPP_

#include <list>
#include <set>
#include <string>

#include "clustering/administration/issues/global.hpp"
#include "clustering/administration/issues/json.hpp"
#include "clustering/administration/metadata.hpp"
#include "http/json.hpp"
#include "rpc/semilattice/view.hpp"

template <class protocol_t>
class pinnings_shards_mismatch_issue_t : public global_issue_t {
public:
    pinnings_shards_mismatch_issue_t(
            const namespace_id_t &offending_namespace,
            const std::set<typename protocol_t::region_t> &shards,
            const region_map_t<protocol_t, uuid_t> &primary_pinnings,
            const region_map_t<protocol_t, std::set<uuid_t> > &secondary_pinnings);

    std::string get_description() const;

    cJSON *get_json_description();

    pinnings_shards_mismatch_issue_t *clone() const;

    namespace_id_t offending_namespace;
    std::set<typename protocol_t::region_t> shards;
    region_map_t<protocol_t, machine_id_t> primary_pinnings;
    region_map_t<protocol_t, std::set<machine_id_t> > secondary_pinnings;

private:
    DISABLE_COPYING(pinnings_shards_mismatch_issue_t);
};

template <class protocol_t>
class pinnings_shards_mismatch_issue_tracker_t : public global_issue_tracker_t {
public:
    explicit pinnings_shards_mismatch_issue_tracker_t(
            boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > > _semilattice_view) :
        semilattice_view(_semilattice_view) { }

    std::list<clone_ptr_t<global_issue_t> > get_issues();

private:
    boost::shared_ptr<semilattice_read_view_t<cow_ptr_t<namespaces_semilattice_metadata_t<protocol_t> > > > semilattice_view;

    DISABLE_COPYING(pinnings_shards_mismatch_issue_tracker_t);
};

#endif  // CLUSTERING_ADMINISTRATION_ISSUES_PINNINGS_SHARDS_MISMATCH_HPP_
