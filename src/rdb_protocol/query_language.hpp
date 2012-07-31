#ifndef RDB_PROTOCOL_QUERY_LANGUAGE_HPP_
#define RDB_PROTOCOL_QUERY_LANGUAGE_HPP_

#include <list>
#include <deque>

#include "utils.hpp"
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "rdb_protocol/backtrace.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.pb.h"
#include "rdb_protocol/scope.hpp"
#include "rdb_protocol/stream.hpp"

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

/* These functions throw exceptions if their inputs aren't well defined or
fail type-checking. (A well-defined input has the correct fields filled in.) */

term_info_t get_term_type(const Term &t, type_checking_environment_t *env, const backtrace_t &backtrace);
void check_term_type(const Term &t, term_type_t expected, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);
term_info_t get_function_type(const Term::Call &c, type_checking_environment_t *env, const backtrace_t &backtrace);
void check_reduction_type(const Reduction &m, type_checking_environment_t *env, bool *is_det_out, bool args_are_det, const backtrace_t &backtrace);
void check_mapping_type(const Mapping &m, term_type_t return_type, type_checking_environment_t *env, bool *is_det_out, bool args_are_det, const backtrace_t &backtrace);
void check_predicate_type(const Predicate &m, type_checking_environment_t *env, bool *is_det_out, bool args_are_det, const backtrace_t &backtrace);
void check_read_query_type(const ReadQuery &rq, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);
void check_write_query_type(const WriteQuery &wq, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);
void check_query_type(const Query &q, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);

/* functions to evaluate the queries */


//Scopes for single pieces of json
typedef variable_scope_t<boost::shared_ptr<scoped_cJSON_t> > variable_val_scope_t;

typedef variable_val_scope_t::new_scope_t new_val_scope_t;

//scopes for json streams
typedef variable_scope_t<boost::shared_ptr<stream_multiplexer_t> > variable_stream_scope_t;

typedef variable_stream_scope_t::new_scope_t new_stream_scope_t;

class runtime_environment_t {
public:
    runtime_environment_t(extproc::pool_group_t *_pool_group,
                          namespace_repo_t<rdb_protocol_t> *_ns_repo,
                          boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > _semilattice_metadata,
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
    boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> > semilattice_metadata;

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

typedef implicit_value_t<boost::shared_ptr<scoped_cJSON_t> >::impliciter_t implicit_value_setter_t;

//TODO most of these functions that are supposed to only throw runtime exceptions

void execute(Query *q, runtime_environment_t *, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

void execute(ReadQuery *r, runtime_environment_t *, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

void execute(WriteQuery *r, runtime_environment_t *, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

boost::shared_ptr<scoped_cJSON_t> eval(Term *t, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

boost::shared_ptr<json_stream_t> eval_stream(Term *t, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

boost::shared_ptr<scoped_cJSON_t> eval(Term::Call *c, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

boost::shared_ptr<json_stream_t> eval_stream(Term::Call *c, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

namespace_repo_t<rdb_protocol_t>::access_t eval(TableRef *t, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

class view_t {
public:
    view_t(const namespace_repo_t<rdb_protocol_t>::access_t &_access,
           const std::string &_primary_key,
           boost::shared_ptr<json_stream_t> _stream)
        : access(_access), primary_key(_primary_key), stream(_stream) 
    { }

    namespace_repo_t<rdb_protocol_t>::access_t access;
    std::string primary_key;
    boost::shared_ptr<json_stream_t> stream;
};

view_t eval_view(Term *t, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

view_t eval_view(Term::Call *t, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

view_t eval_view(Term::Table *t, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t);

} //namespace query_language

#endif /* RDB_PROTOCOL_QUERY_LANGUAGE_HPP_ */
