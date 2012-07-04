#ifndef CLUSTERING_ADMINISTRATION_PERFMON_COLLECTION_REPO_HPP_
#define CLUSTERING_ADMINISTRATION_PERFMON_COLLECTION_REPO_HPP_

#include "perfmon/perfmon.hpp"
#include "clustering/administration/namespace_metadata.hpp"

class perfmon_collection_repo_t {
public:
    explicit perfmon_collection_repo_t(perfmon_collection_t *);
    perfmon_collection_t *get_perfmon_collection_for_namespace(namespace_id_t);
private:
    perfmon_collection_t *parent;
    boost::ptr_map<namespace_id_t, perfmon_membership_t> perfmon_collections;
};

#endif /* CLUSTERING_ADMINISTRATION_PERFMON_COLLECTION_REPO_HPP_ */
