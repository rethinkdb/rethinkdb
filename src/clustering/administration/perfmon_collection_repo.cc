// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "clustering/administration/perfmon_collection_repo.hpp"

perfmon_collection_repo_t::collections_t::collections_t(perfmon_collection_t *_parent, namespace_id_t id)
    : namespace_collection(), serializers_collection(),
      parent_membership(_parent, &namespace_collection, uuid_to_str(id)),
      namespace_membership(&namespace_collection, &serializers_collection, "serializers")
{
}

perfmon_collection_repo_t::perfmon_collection_repo_t(perfmon_collection_t *_parent)
    : parent(_parent)
{ }

perfmon_collection_repo_t::collections_t *perfmon_collection_repo_t::get_perfmon_collections_for_namespace(namespace_id_t n_id) {
    if (!std_contains(perfmon_collections, n_id)) {
        perfmon_collections.insert(n_id, new collections_t(parent, n_id));
    }
    return perfmon_collections.find(n_id)->second;
}
