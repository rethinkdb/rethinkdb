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
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "rdb_protocol/backtrace.hpp"
#include "rdb_protocol/exceptions.hpp"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/query_language.pb.h"
#include "rdb_protocol/scope.hpp"
#include "rdb_protocol/stream.hpp"
#include "rdb_protocol/environment.hpp"

namespace query_language {

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
//TODO most of these functions that are supposed to only throw runtime exceptions

void execute(Query *q, runtime_environment_t *, Response *res, const backtrace_t &backtrace, stream_cache_t *stream_cache) THROWS_ONLY(runtime_exc_t, broken_client_exc_t);

void execute(ReadQuery *r, runtime_environment_t *, Response *res, const backtrace_t &backtrace, stream_cache_t *stream_cache) THROWS_ONLY(runtime_exc_t, broken_client_exc_t);

void execute(WriteQuery *r, runtime_environment_t *, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t, broken_client_exc_t);

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
