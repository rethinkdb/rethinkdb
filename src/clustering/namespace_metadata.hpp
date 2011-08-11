#ifndef __CLUSTERING_NAMESPACE_HPP__
#define __CLUSTERING_NAMESPACE_HPP__

template<class protocol_t>
struct namespace_metadata_t {
    std::map<branch_id_t, branch_metadata_t> branches;
};

#endif /* __CLUSTERING_NAMESPACE_HPP__ */
