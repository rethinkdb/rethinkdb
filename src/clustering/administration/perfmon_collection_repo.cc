#include "clustering/administration/perfmon_collection_repo.hpp"

perfmon_collection_repo_t::perfmon_collection_repo_t(perfmon_collection_t *_parent)
    : parent(_parent)
{ }

perfmon_collection_t *perfmon_collection_repo_t::get_perfmon_collection_for_namespace(namespace_id_t n_id) {
    if (!std_contains(perfmon_collections, n_id)) {
        perfmon_collection_t *collection = new perfmon_collection_t();
        perfmon_membership_t *membership = new perfmon_membership_t(parent, collection, uuid_to_str(n_id), true);
        perfmon_collections.insert(n_id, membership);
    }
    return static_cast<perfmon_collection_t*>(perfmon_collections.find(n_id)->second->get());
}
