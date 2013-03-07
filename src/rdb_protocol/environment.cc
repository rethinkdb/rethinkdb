// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/environment.hpp"

#include "extproc/pool.hpp"

namespace query_language {

runtime_environment_t::runtime_environment_t(
    extproc::pool_group_t *_pool_group,
    namespace_repo_t<rdb_protocol_t> *_ns_repo,
    clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > >
         _namespaces_semilattice_metadata,
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
         _databases_semilattice_metadata,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        _semilattice_metadata,
    directory_read_manager_t<cluster_directory_metadata_t> *_directory_read_manager,
    boost::shared_ptr<js::runner_t> _js_runner,
    signal_t *_interruptor,
    uuid_u _this_machine)
    : pool(_pool_group->get()),
      ns_repo(_ns_repo),
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
      databases_semilattice_metadata(_databases_semilattice_metadata),
      semilattice_metadata(_semilattice_metadata),
      directory_read_manager(_directory_read_manager),
      js_runner(_js_runner),
      interruptor(_interruptor),
      this_machine(_this_machine) {
    guarantee(js_runner);
}

runtime_environment_t::runtime_environment_t(
    extproc::pool_group_t *_pool_group,
    namespace_repo_t<rdb_protocol_t> *_ns_repo,
    clone_ptr_t<watchable_t<cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > >
         _namespaces_semilattice_metadata,
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
         _databases_semilattice_metadata,
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        _semilattice_metadata,
    boost::shared_ptr<js::runner_t> _js_runner,
    signal_t *_interruptor,
    uuid_u _this_machine)
    : pool(_pool_group->get()),
      ns_repo(_ns_repo),
      namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
      databases_semilattice_metadata(_databases_semilattice_metadata),
      semilattice_metadata(_semilattice_metadata),
      directory_read_manager(NULL),
      js_runner(_js_runner),
      interruptor(_interruptor),
      this_machine(_this_machine) {
    guarantee(js_runner);
}




}  // namespace query_language
