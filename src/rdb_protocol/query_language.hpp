// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_QUERY_LANGUAGE_HPP_
#define RDB_PROTOCOL_QUERY_LANGUAGE_HPP_

#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "utils.hpp"
#include <boost/variant.hpp>
#include <boost/shared_ptr.hpp>

#include "clustering/administration/metadata.hpp"
#include "clustering/administration/namespace_interface_repository.hpp"
#include "http/json.hpp"
#include "rdb_protocol/bt.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.pb.h"
#include "rdb_protocol/scope.hpp"
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/environment.hpp"

void wait_for_rdb_table_readiness(namespace_repo_t<rdb_protocol_t> *ns_repo, namespace_id_t namespace_id, signal_t *interruptor, boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata) THROWS_ONLY(interrupted_exc_t);

namespace query_language {

/* These functions throw exceptions if their inputs aren't well defined or
fail type-checking. (A well-defined input has the correct fields filled in.) */

point_modify_ns::result_t calculate_modify(boost::shared_ptr<scoped_cJSON_t> lhs, const std::string &primary_key, point_modify_ns::op_t op,
                                           const Mapping &mapping, runtime_environment_t *env, const scopes_t &scopes,
                                           const backtrace_t &backtrace, boost::shared_ptr<scoped_cJSON_t> *json_out,
                                           std::string *new_key_out) THROWS_ONLY(runtime_exc_t);

/* functions to evaluate the queries */
// TODO most of these functions that are supposed to only throw runtime exceptions
// TODO some of these functions may be called from rdb_protocol/btree.cc, which can only handle runtime exceptions

void execute_write_query(WriteQuery3 *r, runtime_environment_t *, Response3 *res, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t);

boost::shared_ptr<scoped_cJSON_t> eval_term_as_json(Term3 *t, runtime_environment_t *, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t);

boost::shared_ptr<json_stream_t> eval_term_as_stream(Term3 *t, runtime_environment_t *, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t);


class predicate_t {
public:
    predicate_t(const Predicate &_pred, runtime_environment_t *_env, const scopes_t &_scopes, const backtrace_t &_backtrace);
    bool operator()(boost::shared_ptr<scoped_cJSON_t> json);
private:
    Predicate pred;
    runtime_environment_t *env;
    scopes_t scopes;
    backtrace_t backtrace;
};


boost::shared_ptr<scoped_cJSON_t> map_rdb(std::string arg, Term3 *term, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, boost::shared_ptr<scoped_cJSON_t> val);

boost::shared_ptr<json_stream_t> concatmap(std::string arg, Term3 *term, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, boost::shared_ptr<scoped_cJSON_t> val);

} //namespace query_language

#endif /* RDB_PROTOCOL_QUERY_LANGUAGE_HPP_ */
