#ifndef RDB_PROTOCOL_ENVIRONMENT_HPP_
#define RDB_PROTOCOL_ENVIRONMENT_HPP_

#include <map>

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/metadata.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/serializable_environment.hpp"
#include "rdb_protocol/stream.hpp"

namespace query_language {

class runtime_environment_t {
public:
    runtime_environment_t(
        extproc::pool_group_t *_pool_group,
        namespace_repo_t<rdb_protocol_t> *_ns_repo,
        clone_ptr_t<watchable_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > >
             _namespaces_semilattice_metadata,
        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >
            _directory_metadata,
        boost::shared_ptr<js::runner_t> _js_runner,
        signal_t *_interruptor,
        uuid_t _this_machine)
        : pool(_pool_group->get()),
          ns_repo(_ns_repo),
          namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
          databases_semilattice_metadata(_databases_semilattice_metadata),
          semilattice_metadata(_semilattice_metadata),
          directory_metadata(_directory_metadata),
          js_runner(_js_runner),
          interruptor(_interruptor),
          this_machine(_this_machine)
    { }

    runtime_environment_t(
        extproc::pool_group_t *_pool_group,
        namespace_repo_t<rdb_protocol_t> *_ns_repo,
        clone_ptr_t<watchable_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > >
             _namespaces_semilattice_metadata,
        clone_ptr_t<watchable_t<databases_semilattice_metadata_t> >
             _databases_semilattice_metadata,
        boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
            _semilattice_metadata,
        boost::shared_ptr<js::runner_t> _js_runner,
        signal_t *_interruptor,
        uuid_t _this_machine)
        : pool(_pool_group->get()),
          ns_repo(_ns_repo),
          namespaces_semilattice_metadata(_namespaces_semilattice_metadata),
          databases_semilattice_metadata(_databases_semilattice_metadata),
          semilattice_metadata(_semilattice_metadata),
          js_runner(_js_runner),
          interruptor(_interruptor),
          this_machine(_this_machine)
    { }

    /* This is put in a seperate structure so that it can be made serializable. */
    scopes_t scopes;

    extproc::pool_t *pool;      // for running external JS jobs
    namespace_repo_t<rdb_protocol_t> *ns_repo;

    clone_ptr_t<watchable_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > namespaces_semilattice_metadata;
    clone_ptr_t<watchable_t<databases_semilattice_metadata_t> > databases_semilattice_metadata;
    //TODO this should really just be the namespace metadata... but
    //constructing views is too hard :-/
    boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> >
        semilattice_metadata;
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > >
        directory_metadata;

  private:
    // Ideally this would be a scoped_ptr_t<js::runner_t>, but unfortunately we
    // copy runtime_environment_ts to capture scope.
    //
    // Note that js_runner is "lazily initialized": we only call
    // js_runner->begin() once we know we need to evaluate javascript. This
    // means we only allocate a worker process to queries that actually need
    // javascript execution.
    //
    // In the future we might want to be even finer-grained than this, and
    // release worker jobs once we know we no longer need JS execution, or
    // multiplex queries onto worker processes.
    boost::shared_ptr<js::runner_t> js_runner;

  public:
    // Returns js_runner, but first calls js_runner->begin() if it hasn't
    // already been called.
    boost::shared_ptr<js::runner_t> get_js_runner();

    signal_t *interruptor;
    uuid_t this_machine;
};

} //namespace query_language

#endif  // RDB_PROTOCOL_ENVIRONMENT_HPP_
