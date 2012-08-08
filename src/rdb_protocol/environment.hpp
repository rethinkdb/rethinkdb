#ifndef RDB_PROTOCOL_ENVIRONMENT_HPP_
#define RDB_PROTOCOL_ENVIRONMENT_HPP_

#include "rdb_protocol/scope.hpp"
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/js.hpp"

namespace query_language {

enum term_type_t {
    TERM_TYPE_JSON,
    TERM_TYPE_STREAM,
    TERM_TYPE_VIEW,

    /* This is the type of `Error` terms. It's called "arbitrary" because an
    `Error` term can be either a stream or an object. It is a subtype of every
    type. */
    TERM_TYPE_ARBITRARY
};

class term_info_t {
public:
    term_info_t() { }
    term_info_t(term_type_t _type, bool _deterministic)
        : type(_type), deterministic(_deterministic)
    { }

    term_type_t type;
    bool deterministic;
};

typedef variable_scope_t<term_info_t> variable_type_scope_t;

typedef variable_type_scope_t::new_scope_t new_scope_t;

typedef implicit_value_t<term_info_t> implicit_type_t;

struct type_checking_environment_t {
    variable_type_scope_t scope;
    implicit_type_t implicit_type;
};

//Scopes for single pieces of json
typedef variable_scope_t<boost::shared_ptr<scoped_cJSON_t> > variable_val_scope_t;

typedef variable_val_scope_t::new_scope_t new_val_scope_t;

//scopes for json streams
typedef variable_scope_t<boost::shared_ptr<stream_multiplexer_t> > variable_stream_scope_t;

typedef variable_stream_scope_t::new_scope_t new_stream_scope_t;

typedef implicit_value_t<boost::shared_ptr<scoped_cJSON_t> >::impliciter_t implicit_value_setter_t;

class runtime_environment_t {
public:
    runtime_environment_t(extproc::pool_group_t *_pool_group,
                          namespace_repo_t<rdb_protocol_t> *_ns_repo,
                          boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > _semilattice_metadata,
                          boost::shared_ptr<js::runner_t> _js_runner,
                          signal_t *_interruptor)
        : pool(_pool_group->get()),
          ns_repo(_ns_repo),
          semilattice_metadata(_semilattice_metadata),
          js_runner(_js_runner),
          interruptor(_interruptor)
    { }

    variable_val_scope_t scope;
    variable_stream_scope_t stream_scope;
    type_checking_environment_t type_env;

    implicit_value_t<boost::shared_ptr<scoped_cJSON_t> > implicit_attribute_value;

    extproc::pool_t *pool;      // for running external JS jobs
    namespace_repo_t<rdb_protocol_t> *ns_repo;
    //TODO this should really just be the namespace metadata... but
    //constructing views is too hard :-/
    boost::shared_ptr<semilattice_read_view_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > > semilattice_metadata;

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
};

} //namespace query_language 

#endif  // RDB_PROTOCOL_ENVIRONMENT_HPP_
