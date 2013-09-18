// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_MAIN_FILE_BASED_SVS_BY_NAMESPACE_HPP_
#define CLUSTERING_ADMINISTRATION_MAIN_FILE_BASED_SVS_BY_NAMESPACE_HPP_

#include <string>

#include "clustering/administration/reactor_driver.hpp"

template <class protocol_t>
class file_based_svs_by_namespace_t : public svs_by_namespace_t<protocol_t> {
public:
    file_based_svs_by_namespace_t(io_backender_t *io_backender,
                                  const base_path_t& base_path)
        : io_backender_(io_backender), base_path_(base_path), thread_counter_(0) { }

    void get_svs(perfmon_collection_t *serializers_perfmon_collection,
                 namespace_id_t namespace_id,
                 int64_t cache_size,
                 stores_lifetimer_t<protocol_t> *stores_out,
                 scoped_ptr_t<multistore_ptr_t<protocol_t> > *svs_out,
                 typename protocol_t::context_t *);

    void destroy_svs(namespace_id_t namespace_id);

    serializer_filepath_t file_name_for(namespace_id_t namespace_id);

private:
    io_backender_t *io_backender_;
    const base_path_t base_path_;

    threadnum_t next_thread(int num_db_threads);
    int thread_counter_; // should only be used by `next_thread`

    DISABLE_COPYING(file_based_svs_by_namespace_t);
};

#endif  // CLUSTERING_ADMINISTRATION_MAIN_FILE_BASED_SVS_BY_NAMESPACE_HPP_
