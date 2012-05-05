#include "clustering/administration/perfmon_collection_repo.hpp"

perfmon_collection_repo_t::perfmon_collection_repo_t(perfmon_collection_t *_parent)
    : parent(_parent)
{ }

perfmon_collection_t *perfmon_collection_repo_t::get_perfmon_collection_for_namespace(namespace_id_t n_id) {
    if (!std_contains(perfmon_collections, n_id)) {
        perfmon_collections.insert(n_id, new perfmon_collection_t(uuid_to_str(n_id), parent, true, true));
    }
    return perfmon_collections.find(n_id)->second;
}
