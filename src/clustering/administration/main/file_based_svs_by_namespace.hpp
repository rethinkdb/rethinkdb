#ifndef CLUSTERING_ADMINISTRATION_MAIN_FILE_BASED_SVS_BY_NAMESPACE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_FILE_BASED_SVS_BY_NAMESPACE_HPP_

#include <string>

#include "clustering/administration/reactor_driver.hpp"

template <class protocol_t>
class file_based_svs_by_namespace_t : public svs_by_namespace_t<protocol_t> {
public:
    file_based_svs_by_namespace_t(io_backender_t *io_backender, const std::string &file_path) : io_backender_(io_backender), file_path_(file_path) { }

    void get_svs(perfmon_collection_t *serializers_perfmon_collection, namespace_id_t namespace_id,
                 stores_lifetimer_t<protocol_t> *stores_out,
                 scoped_ptr_t<multistore_ptr_t<protocol_t> > *svs_out,
                 typename protocol_t::context_t *);

    void destroy_svs(namespace_id_t namespace_id);

    std::string file_name_for(namespace_id_t namespace_id, int i);

private:

    io_backender_t *io_backender_;
    const std::string file_path_;

    DISABLE_COPYING(file_based_svs_by_namespace_t);
};

#endif  // CLUSTERING_ADMINISTRATION_MAIN_FILE_BASED_SVS_BY_NAMESPACE_HPP_
