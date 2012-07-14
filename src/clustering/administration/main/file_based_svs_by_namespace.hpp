#ifndef CLUSTERING_ADMINISTRATION_MAIN_FILE_BASED_SVS_BY_NAMESPACE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_FILE_BASED_SVS_BY_NAMESPACE_HPP_

#include <string>

#include "clustering/administration/reactor_driver.hpp"




template <class protocol_t>
class file_based_svs_by_namespace_t : public svs_by_namespace_t<protocol_t> {
public:
    explicit file_based_svs_by_namespace_t(const std::string &file_path) : file_path_(file_path) { }

    void get_svs(perfmon_collection_t *perfmon_collection, namespace_id_t namespace_id,
                 boost::scoped_array<boost::scoped_ptr<typename protocol_t::store_t> > *stores_out,
                 boost::scoped_ptr<multistore_ptr_t<protocol_t> > *svs_out);

    void destroy_svs(namespace_id_t namespace_id);

private:
    const std::string file_path_;

    DISABLE_COPYING(file_based_svs_by_namespace_t);
};



#endif  // CLUSTERING_ADMINISTRATION_MAIN_FILE_BASED_SVS_BY_NAMESPACE_HPP_
