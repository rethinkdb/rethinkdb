#ifndef __CLUSTERING_ADMINISTRATION_PERFMON_COLLECTION_REPO_HPP__
#define __CLUSTERING_ADMINISTRATION_PERFMON_COLLECTION_REPO_HPP__

#include "perfmon.hpp"
#include "clustering/administration/namespace_metadata.hpp"

class perfmon_collection_repo_t {
public:
    perfmon_collection_repo_t(perfmon_collection_t *);
    perfmon_collection_t *get_perfmon_collection_for_namespace(namespace_id_t);
private:
    perfmon_collection_t *parent;
    boost::ptr_map<namespace_id_t, perfmon_collection_t> perfmon_collections;
};

#endif
