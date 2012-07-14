#ifndef CLUSTERING_ADMINISTRATION_PERFMON_COLLECTION_REPO_HPP_
#define CLUSTERING_ADMINISTRATION_PERFMON_COLLECTION_REPO_HPP_

#include "perfmon/perfmon.hpp"
#include "clustering/administration/namespace_metadata.hpp"

class perfmon_collection_repo_t {
public:
    class collections_t {
    public:
        collections_t(perfmon_collection_t *parent, namespace_id_t id);

        perfmon_collection_t namespace_collection;
        perfmon_collection_t serializers_collection;
    private:
        perfmon_membership_t parent_membership;
        perfmon_membership_t namespace_membership;
    };
    explicit perfmon_collection_repo_t(perfmon_collection_t *);
    collections_t *get_perfmon_collections_for_namespace(namespace_id_t);

private:
    perfmon_collection_t *parent;
    boost::ptr_map<namespace_id_t, collections_t> perfmon_collections;

    DISABLE_COPYING(perfmon_collection_repo_t);
};

#endif /* CLUSTERING_ADMINISTRATION_PERFMON_COLLECTION_REPO_HPP_ */
