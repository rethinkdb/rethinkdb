#ifndef CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP_
#define CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP_

#include "clustering/immediate_consistency/branch/metadata.hpp"

template <class protocol_t>
class branch_history_manager_t : public home_thread_mixin_t {
public:
    /* Returns information about one specific branch. Crashes if we don't have a
    record for this branch. */
    virtual branch_birth_certificate_t<protocol_t> get_branch(branch_id_t branch) THROWS_NOTHING = 0;

    /* Adds a new branch to the database. Blocks until it is safely on disk. */
    virtual void create_branch(branch_id_t branch_id, const branch_birth_certificate_t<protocol_t> &bc, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

    /* Copies records related to the given branch and all its ancestors into
    `out`. The reason this mutates `out` instead of returning a
    `branch_history_t` is so that you can call it several times with different
    branches that share history. */
    virtual void export_branch_history(branch_id_t branch, branch_history_t<protocol_t> *out) THROWS_NOTHING = 0;

    /* Convenience function that finds all records related to the given version
    map and copies them into `out` */
    void export_branch_history(const region_map_t<protocol_t, version_range_t> &region_map, branch_history_t<protocol_t> *out) THROWS_NOTHING {
        for (typename region_map_t<protocol_t, version_range_t>::const_iterator it = region_map.begin(); it != region_map.end(); it++) {
            if (!it->second.latest.branch.is_nil()) {
                export_branch_history(it->second.latest.branch, out);
            }
        }
    }

    /* Stores the given branch history records. Blocks until they are safely on
    disk. */
    virtual void import_branch_history(const branch_history_t<protocol_t> &new_records, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;
};

template <class protocol_t>
bool version_is_ancestor(
        branch_history_manager_t<protocol_t> *branch_history_manager,
        version_t ancestor,
        version_t descendent,
        typename protocol_t::region_t relevant_region);

template <class protocol_t>
bool version_is_divergent(
        branch_history_manager_t<protocol_t> *branch_history_manager,
        version_t v1,
        version_t v2,
        typename protocol_t::region_t relevant_region);

#endif /* CLUSTERING_IMMEDIATE_CONSISTENCY_BRANCH_HISTORY_HPP_ */
