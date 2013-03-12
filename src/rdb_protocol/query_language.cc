// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/query_language.hpp"

#include <cmath>

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/variant.hpp>

#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/suggester.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "extproc/pool.hpp"
#include "http/json.hpp"
#include "rdb_protocol/internal_extensions.pb.h"
#include "rdb_protocol/js.hpp"
#include "rdb_protocol/stream_cache.hpp"
#include "rdb_protocol/proto_utils.hpp"
#include "rpc/directory/read_manager.hpp"


//TODO: why is this not in the query_language namespace? - because it's also used by rethinkdb import at the moment
void wait_for_rdb_table_readiness(namespace_repo_t<rdb_protocol_t> *ns_repo,
                                  namespace_id_t namespace_id,
                                  signal_t *interruptor,
                                  boost::shared_ptr<semilattice_readwrite_view_t<cluster_semilattice_metadata_t> > semilattice_metadata) THROWS_ONLY(interrupted_exc_t) {
    /* The following is an ugly hack, but it's probably what we want.  It
       takes about a third of a second for the new namespace to get to the
       point where we can do reads/writes on it.  We don't want to return
       until that has happened, so we try to do a read every `poll_ms`
       milliseconds until one succeeds, then return. */

    // This read won't succeed, but we care whether it fails with an exception.
    // It must be an rget to make sure that access is available to all shards.

    const int poll_ms = 10;
    rdb_protocol_t::rget_read_t empty_rget_read(hash_region_t<key_range_t>::universe());
    rdb_protocol_t::read_t empty_read(empty_rget_read);
    for (;;) {
        signal_timer_t start_poll(poll_ms);
        wait_interruptible(&start_poll, interruptor);
        try {
            // Make sure the namespace still exists in the metadata, if not, abort
            {
                // TODO: use a cross thread watchable instead?  not exactly pressed for time here...
                on_thread_t rethread(semilattice_metadata->home_thread());
                cluster_semilattice_metadata_t metadata = semilattice_metadata->get();
                cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&metadata.rdb_namespaces);
                std::map<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<rdb_protocol_t> > >::iterator
                    nsi = change.get()->namespaces.find(namespace_id);
                rassert(nsi != change.get()->namespaces.end());
                if (nsi->second.is_deleted()) throw interrupted_exc_t();
            }

            namespace_repo_t<rdb_protocol_t>::access_t ns_access(ns_repo, namespace_id, interruptor);
            rdb_protocol_t::read_response_t read_res;
            // TODO: We should not use order_token_t::ignore.
            ns_access.get_namespace_if()->read(empty_read, &read_res, order_token_t::ignore, interruptor);
            break;
        } catch (cannot_perform_query_exc_t e) { } //continue loop
    }
}

namespace query_language {

term_info_t get_term_type(Term3 *t, type_checking_environment_t *env, const backtrace_t &backtrace);
void check_term_type(Term3 *t, term_type_t expected, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);
void check_reduction_type(Reduction *m, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);
void check_mapping_type(Mapping *m, term_type_t return_type, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);
void check_predicate_type(Predicate *m, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);
void check_read_query_type(ReadQuery3 *rq, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);
void check_query_type(Query3 *q, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace);


namespace_repo_t<rdb_protocol_t>::access_t eval_table_ref(TableRef *t, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t);



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

view_t eval_term_as_view(Term3 *t, runtime_environment_t *, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t);

view_t eval_table_as_view(Term3::Table *t, runtime_environment_t *, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t);

boost::shared_ptr<scoped_cJSON_t> eval_mapping(Mapping m, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace,
                                               boost::shared_ptr<scoped_cJSON_t> val);





cJSON *safe_cJSON_CreateNumber(double d, const backtrace_t &backtrace) {
    if (!std::isfinite(d)) throw runtime_exc_t(strprintf("Illegal numeric value %e.", d), backtrace);
    return cJSON_CreateNumber(d);
}
#define cJSON_CreateNumber(...) CT_ASSERT(!"Use safe_cJSON_CreateNumber")
#ifdef cJSON_CreateNumber //So the compiler won't complain that it's unused
#endif

/* Convenience function for making the horrible easy. */
boost::shared_ptr<scoped_cJSON_t> shared_scoped_json(cJSON *json) {
    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(json));
}

//TODO: this should really return more informat
void check_protobuf(bool cond) {
    if (!cond) {
        throw broken_client_exc_t("bad protocol buffer; client is buggy");
    }
}

std::string term_type_name(term_type_t tt) {
    switch (tt) {
        case TERM_TYPE_JSON:
            return "object";
        case TERM_TYPE_STREAM:
            return "stream";
        case TERM_TYPE_VIEW:
            return "view";
        case TERM_TYPE_ARBITRARY:
            return "arbitrary";
        default:
            unreachable();
    }
}

bool term_type_is_convertible(term_type_t input, term_type_t desired) {
    switch (input) {
        case TERM_TYPE_ARBITRARY:
            return true;
        case TERM_TYPE_JSON:
            return desired == TERM_TYPE_JSON;
        case TERM_TYPE_STREAM:
            return desired == TERM_TYPE_STREAM;
        case TERM_TYPE_VIEW:
            return desired == TERM_TYPE_STREAM || desired == TERM_TYPE_VIEW;
        default:
            unreachable();
    }
}

bool term_type_least_upper_bound(term_info_t left, term_info_t right, term_info_t *out) {
    term_type_t tt;
    if (term_type_is_convertible(left.type, TERM_TYPE_ARBITRARY) &&
            term_type_is_convertible(right.type, TERM_TYPE_ARBITRARY)) {
        tt = TERM_TYPE_ARBITRARY;
    } else if (term_type_is_convertible(left.type, TERM_TYPE_JSON) &&
            term_type_is_convertible(right.type, TERM_TYPE_JSON)) {
        tt = TERM_TYPE_JSON;
    } else if (term_type_is_convertible(left.type, TERM_TYPE_VIEW) &&
            term_type_is_convertible(right.type, TERM_TYPE_VIEW)) {
        tt = TERM_TYPE_VIEW;
    } else if (term_type_is_convertible(left.type, TERM_TYPE_STREAM) &&
            term_type_is_convertible(right.type, TERM_TYPE_STREAM)) {
        tt = TERM_TYPE_STREAM;
    } else {
        return false;
    }

    *out = term_info_t(tt, left.deterministic && right.deterministic);
    return true;
}

void check_name_string(const std::string& name_str, const backtrace_t &backtrace) THROWS_ONLY(bad_query_exc_t) {
    name_string_t name;
    if (!name.assign_value(name_str)) {
        throw bad_query_exc_t(strprintf("Invalid name '%s'.  (%s).", name_str.c_str(), name_string_t::valid_char_msg), backtrace);
    }

}

void check_table_ref(const TableRef &tr, const backtrace_t &backtrace) THROWS_ONLY(bad_query_exc_t) {
    check_name_string(tr.db_name(), backtrace.with("db_name"));
    check_name_string(tr.table_name(), backtrace.with("table_name"));
}

term_info_t get_term_type(Term3 *t, type_checking_environment_t *env, const backtrace_t &backtrace) {
    if (t->HasExtension(extension::inferred_type)) {
        guarantee_debug_throw_release(t->HasExtension(extension::deterministic), backtrace);
        int type = t->GetExtension(extension::inferred_type);
        bool det = t->GetExtension(extension::deterministic);
        return term_info_t(static_cast<term_type_t>(type), det);
    }
    check_protobuf(Term3::TermType_IsValid(t->type()));

    std::vector<const google::protobuf::FieldDescriptor *> fields;
    t->GetReflection()->ListFields(*t, &fields);
    int field_count = fields.size();

    check_protobuf(field_count <= 2);

    boost::optional<term_info_t> ret;
    switch (t->type()) {
    case Term3::IMPLICIT_VAR: {
        if (!env->implicit_type.has_value() || env->implicit_type.get_value().type != TERM_TYPE_JSON) {
            throw bad_query_exc_t("No implicit variable in scope", backtrace);
        } else if (!env->implicit_type.depth_is_legal()) {
            throw bad_query_exc_t("Cannot use implicit variable in nested queries.  Name your variables!", backtrace);
        }
        ret.reset(env->implicit_type.get_value());
    } break;
    case Term3::VAR: {
        check_protobuf(t->has_var());
        if (!env->scope.is_in_scope(t->var())) {
            throw bad_query_exc_t(strprintf("symbol '%s' is not in scope", t->var().c_str()), backtrace);
        }
        ret.reset(env->scope.get(t->var()));
    } break;
    case Term3::LET: {
        bool args_are_det = true;
        check_protobuf(t->has_let());
        new_scope_t scope_maker(&env->scope); //create a new scope
        for (int i = 0; i < t->let().binds_size(); ++i) {
            term_info_t argtype = get_term_type(t->mutable_let()->mutable_binds(i)->mutable_term(), env,
                                                backtrace.with(strprintf("bind:%s", t->let().binds(i).var().c_str())));
            args_are_det &= argtype.deterministic;
            if (!term_type_is_convertible(argtype.type, TERM_TYPE_JSON)) {
                throw bad_query_exc_t("Only JSON objects can be stored in variables.  If you must store a stream, "
                                      "use `STREAMTOARRAY` to convert it explicitly (note that this requires loading "
                                      "the entire stream into memory).",
                                      backtrace.with(strprintf("bind:%s", t->let().binds(i).var().c_str())));
            }
            // Variables are always deterministic, because the value is precomputed.   vvvv
            env->scope.put_in_scope(t->let().binds(i).var(), term_info_t(argtype.type, true));
        }
        term_info_t res = get_term_type(t->mutable_let()->mutable_expr(), env, backtrace.with("expr"));
        res.deterministic &= args_are_det;
        ret.reset(res);
    } break;
    case Term3::IF: {
        check_protobuf(t->has_if_());
        bool test_is_det = true;
        check_term_type(t->mutable_if_()->mutable_test(), TERM_TYPE_JSON, env, &test_is_det, backtrace.with("test"));

        term_info_t true_branch = get_term_type(t->mutable_if_()->mutable_true_branch(), env, backtrace.with("true"));

        term_info_t false_branch = get_term_type(t->mutable_if_()->mutable_false_branch(), env, backtrace.with("false"));

        term_info_t combined_type;
        if (!term_type_least_upper_bound(true_branch, false_branch, &combined_type)) {
            throw bad_query_exc_t(strprintf(
                                      "true-branch has type %s, but false-branch has type %s",
                                      term_type_name(true_branch.type).c_str(),
                                      term_type_name(false_branch.type).c_str()
                                  ),
                                  backtrace);
        }

        combined_type.deterministic &= test_is_det;
        ret.reset(combined_type);
    } break;
    case Term3::ERROR: {
        check_protobuf(t->has_error());
        ret.reset(term_info_t(TERM_TYPE_ARBITRARY, true));
    } break;
    case Term3::NUMBER: {
        check_protobuf(t->has_number());
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term3::STRING: {
        check_protobuf(t->has_valuestring());
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term3::JSON: {
        check_protobuf(t->has_jsonstring());
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term3::BOOL: {
        check_protobuf(t->has_valuebool());
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term3::JSON_NULL: {
        check_protobuf(field_count == 1); // null term has only a type field
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term3::ARRAY: {
        if (t->array_size() == 0) { // empty arrays are valid
            check_protobuf(field_count == 1);
        }
        bool deterministic = true;
        for (int i = 0; i < t->array_size(); ++i) {
            check_term_type(t->mutable_array(i), TERM_TYPE_JSON, env, &deterministic, backtrace.with(strprintf("elem:%d", i)));
        }
        ret.reset(term_info_t(TERM_TYPE_JSON, deterministic));
    } break;
    case Term3::OBJECT: {
        if (t->object_size() == 0) { // empty objects are valid
            check_protobuf(field_count == 1);
        }
        bool deterministic = true;
        for (int i = 0; i < t->object_size(); ++i) {
            check_term_type(t->mutable_object(i)->mutable_term(), TERM_TYPE_JSON, env, &deterministic,
                            backtrace.with(strprintf("key:%s", t->object(i).var().c_str())));
        }
        ret.reset(term_info_t(TERM_TYPE_JSON, deterministic));
    } break;
    case Term3::GETBYKEY: {
        check_protobuf(t->has_get_by_key());
        check_table_ref(t->get_by_key().table_ref(), backtrace.with("table_ref"));
        check_term_type(t->mutable_get_by_key()->mutable_key(), TERM_TYPE_JSON, env, NULL, backtrace.with("key"));
        ret.reset(term_info_t(TERM_TYPE_JSON, false));
    } break;
    case Term3::TABLE: {
        check_protobuf(t->has_table());
        // TODO: I don't like or don't "get" how backtraces are supposed to
        // work. Why do backtraces refer to the subfield of the type but not
        // refer to the type itself?  -sam
        // RE: Backtraces sort of suck right now.  We're hoping to redo them.
        // Basically they provide the minimum amount of information necessary to
        // locate the right portion of the query, except when they don't because
        // somebody thought it made more semantic sense to include extra
        // information.
        check_table_ref(t->table().table_ref(), backtrace.with("table_ref"));
        ret.reset(term_info_t(TERM_TYPE_VIEW, false));
    } break;
    case Term3::JAVASCRIPT: {
        check_protobuf(t->has_javascript());
        ret.reset(term_info_t(TERM_TYPE_JSON, false)); // js is never deterministic
    } break;
    default: unreachable("unhandled Term3 case");
    }
    guarantee_debug_throw_release(ret, backtrace);
    t->SetExtension(extension::inferred_type, static_cast<int32_t>(ret->type));
    t->SetExtension(extension::deterministic, ret->deterministic);
    //debugf("%s", t->DebugString().c_str());
    return *ret;
}

void check_term_type(Term3 *t, term_type_t expected, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    term_info_t actual = get_term_type(t, env, backtrace);
    if (expected != TERM_TYPE_ARBITRARY &&
        !term_type_is_convertible(actual.type, expected)) {
        throw bad_query_exc_t(strprintf("expected a %s; got a %s",
                term_type_name(expected).c_str(), term_type_name(actual.type).c_str()),
            backtrace);
    }

    if (is_det_out) {
        *is_det_out &= actual.deterministic;
    }
}

void check_reduction_type(Reduction *r, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    check_term_type(r->mutable_base(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("base"));

    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(r->var1(), term_info_t(TERM_TYPE_JSON, true));
    env->scope.put_in_scope(r->var2(), term_info_t(TERM_TYPE_JSON, true));
    check_term_type(r->mutable_body(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("body"));
}

void check_mapping_type(Mapping *m, term_type_t return_type, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    bool is_det = true;
    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(m->arg(), term_info_t(TERM_TYPE_JSON, true/*det*/));
    check_term_type(m->mutable_body(), return_type, env, &is_det, backtrace);
    *is_det_out &= is_det;
}

void check_predicate_type(Predicate *p, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(p->arg(), term_info_t(TERM_TYPE_JSON, true));
    check_term_type(p->mutable_body(), TERM_TYPE_JSON, env, is_det_out, backtrace);
}

void check_read_query_type(ReadQuery3 *rq, type_checking_environment_t *env, bool *, const backtrace_t &backtrace) {
    /* Read queries could return anything--a view, a stream, a JSON, or an
    error. Views will be automatically converted to streams at evaluation time.
    */
    term_info_t res = get_term_type(rq->mutable_term(), env, backtrace);
    rq->SetExtension(extension::inferred_read_type, static_cast<int32_t>(res.type));
}

void check_meta_query_type(MetaQuery3 *t, const backtrace_t &backtrace) {
    check_protobuf(MetaQuery3::MetaQueryType_IsValid(t->type()));
    switch (t->type()) {
    case MetaQuery3::CREATE_DB:
        check_protobuf(t->has_db_name());
        check_name_string(t->db_name(), backtrace);
        check_protobuf(!t->has_create_table());
        check_protobuf(!t->has_drop_table());
        break;
    case MetaQuery3::DROP_DB:
        check_protobuf(t->has_db_name());
        check_name_string(t->db_name(), backtrace);
        check_protobuf(!t->has_create_table());
        check_protobuf(!t->has_drop_table());
        break;
    case MetaQuery3::LIST_DBS:
        check_protobuf(!t->has_db_name());
        check_protobuf(!t->has_create_table());
        check_protobuf(!t->has_drop_table());
        break;
    case MetaQuery3::CREATE_TABLE: {
        check_protobuf(!t->has_db_name());
        check_protobuf(t->has_create_table());
        if (!t->create_table().datacenter().empty()) {
            check_name_string(t->create_table().datacenter(), backtrace.with("datacenter"));
        }
        check_table_ref(t->create_table().table_ref(), backtrace.with("table_ref"));
        check_protobuf(!t->has_drop_table());
    } break;
    case MetaQuery3::DROP_TABLE:
        check_protobuf(!t->has_db_name());
        check_protobuf(!t->has_create_table());
        check_protobuf(t->has_drop_table());
        check_table_ref(t->drop_table(), backtrace);
        break;
    case MetaQuery3::LIST_TABLES:
        check_protobuf(t->has_db_name());
        check_name_string(t->db_name(), backtrace);
        check_protobuf(!t->has_create_table());
        check_protobuf(!t->has_drop_table());
        break;
    default: unreachable("Unhandled MetaQuery3.");
    }
}

void check_query_type(Query3 *q, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    check_protobuf(Query3::QueryType_IsValid(q->type()));
    switch (q->type()) {
    case Query3::READ:
        check_protobuf(q->has_read_query());
        check_protobuf(!q->has_meta_query());
        check_read_query_type(q->mutable_read_query(), env, is_det_out, backtrace);
        break;
    case Query3::CONTINUE:
        check_protobuf(!q->has_read_query());
        check_protobuf(!q->has_meta_query());
        break;
    case Query3::STOP:
        check_protobuf(!q->has_read_query());
        check_protobuf(!q->has_meta_query());
        break;
    case Query3::META:
        check_protobuf(q->has_meta_query());
        check_protobuf(!q->has_read_query());
        check_meta_query_type(q->mutable_meta_query(), backtrace);
        break;
    default:
        unreachable("unhandled Query3");
    }
}

boost::shared_ptr<js::runner_t> runtime_environment_t::get_js_runner() {
    pool->assert_thread();
    if (!js_runner->connected()) {
        js_runner->begin(pool);
    }
    return js_runner;
}

static void meta_check(metadata_search_status_t status, metadata_search_status_t want,
                       const std::string &operation, const backtrace_t &backtrace) {
    if (status != want) {
        const char *msg;
        switch (status) {
        case METADATA_SUCCESS: msg = "Entry already exists."; break;
        case METADATA_ERR_NONE: msg = "No entry with that name."; break;
        case METADATA_ERR_MULTIPLE: msg = "Multiple entries with that name."; break;
        case METADATA_CONFLICT: msg = "Entry with that name is in conflict."; break;
        default:
            unreachable();
        }
        throw runtime_exc_t(strprintf("Error during operation `%s`: %s",
                                      operation.c_str(), msg), backtrace);
    }
}

template<class T, class U>
uuid_u meta_get_uuid(T searcher, const U& predicate,
                     const std::string &operation, const backtrace_t &backtrace) {
    metadata_search_status_t status;
    typename T::iterator entry = searcher.find_uniq(predicate, &status);
    meta_check(status, METADATA_SUCCESS, operation, backtrace);
    return entry->first;
}

void assign_name(const char *kind_of_name, const std::string &name_str, const backtrace_t& bt, name_string_t *name_out)
    THROWS_ONLY(runtime_exc_t) {
    if (!name_out->assign_value(name_str)) {
        throw runtime_exc_t(strprintf("%s name '%s' invalid. (%s)", kind_of_name, name_str.c_str(), name_string_t::valid_char_msg), bt);
    }
}

void assign_table_name(const std::string &table_name_str, const backtrace_t& bt, name_string_t *table_name_out)
    THROWS_ONLY(runtime_exc_t) {
    assign_name("table", table_name_str, bt, table_name_out);
}

void assign_db_name(const std::string &db_name_str, const backtrace_t& bt, name_string_t *db_name_out)
    THROWS_ONLY(runtime_exc_t) {
    assign_name("database", db_name_str, bt, db_name_out);
}

void assign_dc_name(const std::string &dc_name_str, const backtrace_t& bt, name_string_t *dc_name_out)
    THROWS_ONLY(runtime_exc_t) {
    if (!dc_name_str.empty()) {
        assign_name("datacenter", dc_name_str, bt, dc_name_out);
    }
}


void execute_meta(MetaQuery3 *m, runtime_environment_t *env, Response3 *res, const backtrace_t &bt) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    // This must be performed on the semilattice_metadata's home thread,
    int original_thread = get_thread_id();
    int metadata_home_thread = env->semilattice_metadata->home_thread();
    guarantee(env->directory_read_manager->home_thread() == metadata_home_thread);
    cross_thread_signal_t ct_interruptor(env->interruptor, metadata_home_thread);
    on_thread_t rethreader(metadata_home_thread);
    clone_ptr_t<watchable_t<std::map<peer_id_t, cluster_directory_metadata_t> > > directory_metadata =
        env->directory_read_manager->get_root_view();

    cluster_semilattice_metadata_t metadata = env->semilattice_metadata->get();
    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t ns_change(&metadata.rdb_namespaces);
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
        ns_searcher(&ns_change.get()->namespaces);
    metadata_searcher_t<database_semilattice_metadata_t>
        db_searcher(&metadata.databases.databases);
    metadata_searcher_t<datacenter_semilattice_metadata_t>
        dc_searcher(&metadata.datacenters.datacenters);

    switch (m->type()) {
    case MetaQuery3::CREATE_DB: {
        name_string_t db_name;
        assign_db_name(m->db_name(), bt.with("db_name"), &db_name);

        /* Ensure database doesn't already exist. */
        metadata_search_status_t status;
        db_searcher.find_uniq(db_name, &status);
        meta_check(status, METADATA_ERR_NONE, "CREATE_DB " + db_name.str(), bt);

        /* Create namespace, insert into metadata, then join into real metadata. */
        database_semilattice_metadata_t db;
        db.name = vclock_t<name_string_t>(db_name, env->this_machine);
        metadata.databases.databases.insert(std::make_pair(generate_uuid(), make_deletable(db)));
        try {
            fill_in_blueprints(&metadata, directory_metadata->get(), env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            throw runtime_exc_t(e.what(), bt);
        }
        env->semilattice_metadata->join(metadata);
        res->set_status_code(Response3::SUCCESS_EMPTY); //return immediately.
    } break;
    case MetaQuery3::DROP_DB: {
        name_string_t db_name;
        assign_db_name(m->db_name(), bt.with("db_anme"), &db_name);

        // Get database metadata.
        metadata_search_status_t status;
        metadata_searcher_t<database_semilattice_metadata_t>::iterator
            db_metadata = db_searcher.find_uniq(db_name, &status);
        meta_check(status, METADATA_SUCCESS, "DROP_DB " + db_name.str(), bt);
        guarantee(!db_metadata->second.is_deleted());
        uuid_u db_id = db_metadata->first;

        // Delete all tables in database.
        namespace_predicate_t pred(&db_id);
        for (metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
                 ::iterator it = ns_searcher.find_next(ns_searcher.begin(), pred);
             it != ns_searcher.end(); it = ns_searcher.find_next(++it, pred)) {
            guarantee(!it->second.is_deleted());
            it->second.mark_deleted();
        }

        // Delete database.
        db_metadata->second.mark_deleted();
        try {
            fill_in_blueprints(&metadata, directory_metadata->get(), env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            throw runtime_exc_t(e.what(), bt);
        }
        env->semilattice_metadata->join(metadata);
        res->set_status_code(Response3::SUCCESS_EMPTY);
    } break;
    case MetaQuery3::LIST_DBS: {
        for (metadata_searcher_t<database_semilattice_metadata_t>::iterator
                 it = db_searcher.find_next(db_searcher.begin());
             it != db_searcher.end(); it = db_searcher.find_next(++it)) {

            // For each undeleted and unconflicted entry in the metadata, add a response.
            guarantee(!it->second.is_deleted());
            if (it->second.get().name.in_conflict()) continue;
            scoped_cJSON_t json(cJSON_CreateString(it->second.get().name.get().c_str()));
            res->add_response(json.PrintUnformatted());
        }
        res->set_status_code(Response3::SUCCESS_STREAM);
    } break;
    case MetaQuery3::CREATE_TABLE: {
        std::string dc_name_str = m->create_table().datacenter();
        name_string_t dc_name;
        assign_dc_name(dc_name_str, bt.with("table_ref").with("dc_name"), &dc_name);
        std::string db_name_str = m->create_table().table_ref().db_name();
        name_string_t db_name;
        assign_db_name(db_name_str, bt.with("table_ref").with("db_name"), &db_name);
        std::string table_name_str = m->create_table().table_ref().table_name();
        name_string_t table_name;
        assign_table_name(table_name_str, bt.with("table_ref").with("table_name"), &table_name);
        std::string primary_key = m->create_table().primary_key();
        int64_t cache_size = m->create_table().cache_size();

        uuid_u db_id = meta_get_uuid(db_searcher, db_name, "FIND_DATABASE " + db_name.str(), bt.with("table_ref").with("db_name"));

        uuid_u dc_id = nil_uuid();
        if (m->create_table().has_datacenter()) {
            dc_id = meta_get_uuid(dc_searcher, dc_name, "FIND_DATACENTER " + dc_name.str(), bt.with("datacenter"));
        }

        namespace_predicate_t search_predicate(&table_name, &db_id);
        /* Ensure table doesn't already exist. */
        metadata_search_status_t status;
        ns_searcher.find_uniq(search_predicate, &status);
        meta_check(status, METADATA_ERR_NONE, "CREATE_TABLE " + table_name.str(), bt);

        /* Create namespace, insert into metadata, then join into real metadata. */
        uuid_u namespace_id = generate_uuid();
        // The port here is a legacy from memcached days when each table was accessed through a different port.
        namespace_semilattice_metadata_t<rdb_protocol_t> ns =
            new_namespace<rdb_protocol_t>(env->this_machine, db_id, dc_id, table_name,
                                          primary_key, port_defaults::reql_port, cache_size);
        {
            cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&metadata.rdb_namespaces);
            change.get()->namespaces.insert(std::make_pair(namespace_id, make_deletable(ns)));
        }
        try {
            fill_in_blueprints(&metadata, directory_metadata->get(), env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            throw runtime_exc_t(e.what(), bt);
        }
        env->semilattice_metadata->join(metadata);

        /* UGLY HACK BELOW.  SEE wait_for_rdb_table_readiness for more info. */

        // This is performed on the client's thread to make sure that any
        //  immediately following queries will succeed.
        on_thread_t rethreader_original(original_thread);
        try {
            wait_for_rdb_table_readiness(env->ns_repo, namespace_id, env->interruptor, env->semilattice_metadata);
        } catch (interrupted_exc_t e) {
            throw runtime_exc_t("Query3 interrupted, probably by user.", bt);
        }
        res->set_status_code(Response3::SUCCESS_EMPTY);
    } break;
    case MetaQuery3::DROP_TABLE: {
        std::string db_name_str = m->drop_table().db_name();
        name_string_t db_name;
        assign_db_name(db_name_str, bt.with("db_name"), &db_name);
        std::string table_name_str = m->drop_table().table_name();
        name_string_t table_name;
        assign_table_name(table_name_str, bt.with("table_name"), &table_name);

        // Get namespace metadata.
        uuid_u db_id = meta_get_uuid(db_searcher, db_name, "FIND_DATABASE " + db_name.str(), bt.with("db_name"));
        namespace_predicate_t search_predicate(&table_name, &db_id);
        metadata_search_status_t status;
        metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >::iterator
            ns_metadata =
            ns_searcher.find_uniq(search_predicate, &status);
        std::string op = strprintf("FIND_TABLE %s.%s", db_name.c_str(), table_name.c_str());
        meta_check(status, METADATA_SUCCESS, op, bt.with("table_name"));
        guarantee(!ns_metadata->second.is_deleted());

        // Delete namespace
        ns_metadata->second.mark_deleted();
        try {
            fill_in_blueprints(&metadata, directory_metadata->get(), env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            throw runtime_exc_t(e.what(), bt);
        }
        env->semilattice_metadata->join(metadata);
        res->set_status_code(Response3::SUCCESS_EMPTY); //return immediately
    } break;
    case MetaQuery3::LIST_TABLES: {
        name_string_t db_name;
        assign_db_name(m->db_name(), bt.with("db_name"), &db_name);
        uuid_u db_id = meta_get_uuid(db_searcher, db_name, "FIND_DATABASE " + db_name.str(), bt.with("db_name"));
        namespace_predicate_t pred(&db_id);
        for (metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
                 ::iterator it = ns_searcher.find_next(ns_searcher.begin(), pred);
             it != ns_searcher.end(); it = ns_searcher.find_next(++it, pred)) {

            // For each undeleted and unconflicted entry in the metadata, add a response.
            guarantee(!it->second.is_deleted());
            if (it->second.get().name.in_conflict()) continue;
            scoped_cJSON_t json(cJSON_CreateString(it->second.get().name.get().c_str()));
            res->add_response(json.PrintUnformatted());
        }
        res->set_status_code(Response3::SUCCESS_STREAM);
    } break;
    default: crash("unreachable");
    }
}

std::string get_primary_key(TableRef *t, runtime_environment_t *env,
                            const backtrace_t &bt) {
    name_string_t db_name;
    assign_db_name(t->db_name(), bt.with("db_name"), &db_name);
    name_string_t table_name;
    assign_table_name(t->table_name(), bt.with("table_name"), &table_name);

    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > ns_metadata = env->namespaces_semilattice_metadata->get();
    databases_semilattice_metadata_t db_metadata = env->databases_semilattice_metadata->get();

    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t ns_metadata_change(&ns_metadata);
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
        ns_searcher(&ns_metadata_change.get()->namespaces);
    metadata_searcher_t<database_semilattice_metadata_t>
        db_searcher(&db_metadata.databases);

    uuid_u db_id = meta_get_uuid(db_searcher, db_name, "FIND_DB " + db_name.str(), bt);
    namespace_predicate_t pred(&table_name, &db_id);
    metadata_search_status_t status;
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >::iterator
        ns_metadata_it = ns_searcher.find_uniq(pred, &status);
    meta_check(status, METADATA_SUCCESS, "FIND_TABLE " + table_name.str(), bt);
    guarantee(!ns_metadata_it->second.is_deleted());
    if (ns_metadata_it->second.get().primary_key.in_conflict()) {
        throw runtime_exc_t(strprintf(
            "Table %s.%s has a primary key in conflict, which should never happen.",
            db_name.c_str(), table_name.c_str()), bt);
    }
    return ns_metadata_it->second.get().primary_key.get();
}

void throwing_insert(namespace_repo_t<rdb_protocol_t>::access_t ns_access, const std::string &pk,
                     boost::shared_ptr<scoped_cJSON_t> data, runtime_environment_t *env,
                     const backtrace_t &backtrace, bool overwrite,
                     boost::optional<std::string> *generated_pk_out) {
    bool generated_key = false;
    if (data->type() != cJSON_Object) {
        throw runtime_exc_t(strprintf("Cannot insert non-object %s", data->Print().c_str()), backtrace);
    }
    if (!data->GetObjectItem(pk.c_str())) {
        if (overwrite) {
            throw runtime_exc_t(strprintf("Must have a field named \"%s\" (The primary key) if you are attempting to overwrite.", pk.c_str()), backtrace);
        }
        std::string generated_pk = uuid_to_str(generate_uuid());
        *generated_pk_out = generated_pk;
        data->AddItemToObject(pk.c_str(), cJSON_CreateString(generated_pk.c_str()));
        generated_key = true;
    }

    cJSON *primary_key = data->GetObjectItem(pk.c_str());
    if (primary_key->type != cJSON_String && primary_key->type != cJSON_Number) {
        throw runtime_exc_t(strprintf("Cannot insert row %s with primary key %s of non-string, non-number type.",
                                      data->Print().c_str(), cJSON_print_std_string(primary_key).c_str()), backtrace);
    }

    try {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t(cJSON_print_primary(data->GetObjectItem(pk.c_str()), backtrace)), data, overwrite));
        rdb_protocol_t::write_response_t response;
        ns_access.get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);

        if (generated_key && boost::get<rdb_protocol_t::point_write_response_t>(response.response).result == DUPLICATE) {
            throw runtime_exc_t("Generated key was a duplicate either you've " \
                    "won the uuid lottery or you've intentionally tried to " \
                    "predict the keys rdb would generate... in which case well " \
                    "done.", backtrace);
        }

        if (!overwrite && boost::get<rdb_protocol_t::point_write_response_t>(response.response).result == DUPLICATE) {
            throw runtime_exc_t(strprintf("Duplicate primary key %s in %s", pk.c_str(), data->Print().c_str()), backtrace);
        }
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

void nonthrowing_insert(namespace_repo_t<rdb_protocol_t>::access_t ns_access, const std::string &pk,
                        boost::shared_ptr<scoped_cJSON_t> data, runtime_environment_t *env,
                        const backtrace_t &backtrace, bool overwrite,
                        std::vector<std::string> *generated_keys,
                        int *inserted, int *errors, std::string *first_error) {
    boost::optional<std::string> generated_key;
    try {
        throwing_insert(ns_access, pk, data, env, backtrace, overwrite, &generated_key);
        *inserted += 1;
    } catch (const runtime_exc_t &e) {
        *errors += 1;
        if (*first_error == "") *first_error = e.as_str();
    }
    if (generated_key) generated_keys->push_back(*generated_key);
}

rdb_protocol_t::point_read_response_t read_by_key(namespace_repo_t<rdb_protocol_t>::access_t ns_access, runtime_environment_t *env,
                                            cJSON *key, bool use_outdated, const backtrace_t &backtrace) {
    rdb_protocol_t::read_t read(rdb_protocol_t::point_read_t(store_key_t(cJSON_print_primary(key, backtrace))));
    rdb_protocol_t::read_response_t res;
    if (use_outdated) {
        ns_access.get_namespace_if()->read_outdated(read, &res, env->interruptor);
    } else {
        ns_access.get_namespace_if()->read(read, &res, order_token_t::ignore, env->interruptor);
    }
    rdb_protocol_t::point_read_response_t *p_res = boost::get<rdb_protocol_t::point_read_response_t>(&res.response);
    guarantee(p_res);
    return *p_res;
}

/* Returns number of rows deleted. */
int point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, cJSON *id, runtime_environment_t *env, const backtrace_t &backtrace) {
    try {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_delete_t(store_key_t(cJSON_print_primary(id, backtrace))));
        rdb_protocol_t::write_response_t response;
        ns_access.get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);
        return (boost::get<rdb_protocol_t::point_delete_response_t>(response.response).result == DELETED);
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

point_modify_ns::result_t calculate_modify(boost::shared_ptr<scoped_cJSON_t> lhs, const std::string &primary_key, point_modify_ns::op_t op,
                                           const Mapping &mapping, runtime_environment_t *env, const scopes_t &scopes,
                                           const backtrace_t &backtrace, boost::shared_ptr<scoped_cJSON_t> *json_out,
                                           std::string *new_key_out) THROWS_ONLY(runtime_exc_t) {
    guarantee(lhs);
    //CASE 1: UPDATE skips nonexistant entries
    if (lhs->type() == cJSON_NULL && op == point_modify_ns::UPDATE) return point_modify_ns::SKIPPED;
    guarantee_debug_throw_release((lhs->type() == cJSON_NULL && op == point_modify_ns::MUTATE)
                                  || lhs->type() == cJSON_Object, backtrace);

    // Evaluate the mapping.
    boost::shared_ptr<scoped_cJSON_t> rhs = eval_mapping(mapping, env, scopes, backtrace, lhs);
    guarantee_debug_throw_release(rhs, backtrace);

    if (rhs->type() == cJSON_NULL) { //CASE 2: if [rhs] is NULL, we either SKIP or DELETE (or NOP if already deleted)
        if (op == point_modify_ns::UPDATE) return point_modify_ns::SKIPPED;
        guarantee(op == point_modify_ns::MUTATE);
        return (lhs->type() == cJSON_NULL) ? point_modify_ns::NOP : point_modify_ns::DELETED;
    } else if (rhs->type() != cJSON_Object) {
        throw runtime_exc_t(strprintf("Got %s, but expected Object.", rhs->Print().c_str()), backtrace);
    }

    // Find the primary keys and if both exist check that they're equal.
    cJSON *lhs_pk = lhs->type() == cJSON_Object ? lhs->GetObjectItem(primary_key.c_str()) : 0;
    guarantee_debug_throw_release(lhs->type() == cJSON_NULL || lhs_pk, backtrace);
    guarantee(rhs->type() == cJSON_Object); //see CASE 2 above
    cJSON *rhs_pk = rhs->GetObjectItem(primary_key.c_str());
    if (lhs_pk && rhs_pk && !cJSON_Equal(lhs_pk, rhs_pk)) { //CHECK 1: Primary key isn't changed
        throw runtime_exc_t(strprintf("%s cannot change primary key %s (got objects %s, %s)",
                                      point_modify_ns::op_name(op).c_str(), primary_key.c_str(),
                                      lhs->Print().c_str(), rhs->Print().c_str()), backtrace);
    }

    if (op == point_modify_ns::MUTATE) {
        *json_out = rhs;
        if (lhs->type() == cJSON_Object) { //CASE 3: a normal MUTATE
            if (!rhs_pk) { //CHECK 2: Primary key isn't removed
                throw runtime_exc_t(strprintf("mutate cannot remove the primary key (%s) of %s (new object: %s)",
                                              primary_key.c_str(), lhs->Print().c_str(), rhs->Print().c_str()), backtrace);
            }
            guarantee(cJSON_Equal(lhs_pk, rhs_pk)); //See CHECK 1 above
            return point_modify_ns::MODIFIED;
        }

        //CASE 4: a deleting MUTATE
        guarantee(lhs->type() == cJSON_NULL);
        if (!rhs_pk) { //CHECK 3: Primary key exists for insert (no generation allowed);
            throw runtime_exc_t(strprintf("mutate must provide a primary key (%s), but there was none in: %s",
                                          primary_key.c_str(), rhs->Print().c_str()), backtrace);
        }
        *new_key_out = cJSON_print_primary(rhs_pk, backtrace); //CHECK 4: valid primary key for insert (may throw)
        return point_modify_ns::INSERTED;
    } else { //CASE 5: a normal UPDATE
        guarantee(op == point_modify_ns::UPDATE && lhs->type() == cJSON_Object);
        json_out->reset(new scoped_cJSON_t(cJSON_merge(lhs->get(), rhs->get())));
        guarantee_debug_throw_release(cJSON_Equal((*json_out)->GetObjectItem(primary_key.c_str()),
                                                  lhs->GetObjectItem(primary_key.c_str())), backtrace);
        return point_modify_ns::MODIFIED;
    }
    //[point_modify::ERROR] never returned; we throw instead and the caller should catch it.
}

point_modify_ns::result_t point_modify(namespace_repo_t<rdb_protocol_t>::access_t ns_access,
                                    const std::string &pk, cJSON *id, point_modify_ns::op_t op,
                                    runtime_environment_t *env, const Mapping &m, const scopes_t &scopes,
                                    bool atomic, boost::shared_ptr<scoped_cJSON_t> row,
                                    const backtrace_t &backtrace) {
    guarantee_debug_throw_release(m.body().HasExtension(extension::deterministic), backtrace);
    bool deterministic = m.body().GetExtension(extension::deterministic);
    point_modify_ns::result_t res;
    std::string opname = point_modify_ns::op_name(op);
    try {
        if (!deterministic) {
            /* Non-deterministic modifications need to be precomputed to ensure that all the replicas end up with the same value. */
            if (atomic) { // Atomic non-deterministic modifications are impossible.
                throw runtime_exc_t(strprintf("Cannot execute atomic %s because the mapping can't be proved deterministic.  "
                                              "Please see the documentation on atomicity and determinism for more details.  "
                                              "If you don't need atomic %s, use %s_nonatomic instead.",
                                              opname.c_str(), opname.c_str(), opname.c_str()), backtrace);
            }
            if (!row) { // Get the row if we don't already know it.
                rdb_protocol_t::point_read_response_t row_read = read_by_key(ns_access, env, id, false, backtrace);
                //TODO: Is it OK to never do an outdated read here?                              ^^^^^
                row = row_read.data;
            }
            boost::shared_ptr<scoped_cJSON_t> new_row;
            std::string new_key;
            res = calculate_modify(row, pk, op, m, env, scopes, backtrace, &new_row, &new_key);
            switch (res) {
            case point_modify_ns::INSERTED:
                if (!cJSON_Equal(new_row->GetObjectItem(pk.c_str()), id)) {
                    throw runtime_exc_t(strprintf("%s can't change the primary key (%s) (original: %s, new row: %s)",
                                                  opname.c_str(), pk.c_str(), cJSON_print_std_string(id).c_str(),
                                                  new_row->Print().c_str()), backtrace);
                }
                //FALLTHROUGH
            case point_modify_ns::MODIFIED: {
                guarantee_debug_throw_release(new_row, backtrace);
                boost::optional<std::string> generated_key;
                throwing_insert(ns_access, pk, new_row, env, backtrace, true /*overwrite*/, &generated_key);
                guarantee(!generated_key); //overwriting inserts never generate keys
            } break;
            case point_modify_ns::DELETED: {
                int num_deleted = point_delete(ns_access, id, env, backtrace);
                if (num_deleted == 0) res = point_modify_ns::NOP;
            } break;
            case point_modify_ns::SKIPPED: break;
            case point_modify_ns::NOP: break;
            case point_modify_ns::ERROR: unreachable("compute_modify should never return ERROR, it should throw");
            default: unreachable();
            }
            return res;
        } else { //deterministic modifications can be dispatched as point_modifies
            rdb_protocol_t::write_t write(rdb_protocol_t::point_modify_t(pk, store_key_t(cJSON_print_primary(id, backtrace)), op, scopes, backtrace, m));
            rdb_protocol_t::write_response_t response;
            ns_access.get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);
            rdb_protocol_t::point_modify_response_t mod_res = boost::get<rdb_protocol_t::point_modify_response_t>(response.response);
            if (mod_res.result == point_modify_ns::ERROR) throw mod_res.exc;
            return mod_res.result;
        }
    } catch (const cannot_perform_query_exc_t &e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

int point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> id, runtime_environment_t *env, const backtrace_t &backtrace) {
    return point_delete(ns_access, id->get(), env, backtrace);
}

//This doesn't create a scope for the evals
void eval_let_binds(Term3::Let *let, runtime_environment_t *env, scopes_t *scopes_in_out, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    // Go through the bindings in a let and add them one by one
    for (int i = 0; i < let->binds_size(); ++i) {
        backtrace_t backtrace_bind = backtrace.with(strprintf("bind:%s", let->binds(i).var().c_str()));
        term_info_t type = get_term_type(let->mutable_binds(i)->mutable_term(), &scopes_in_out->type_env, backtrace_bind);

        if (type.type == TERM_TYPE_JSON) {
            scopes_in_out->scope.put_in_scope(let->binds(i).var(),
                    eval_term_as_json(let->mutable_binds(i)->mutable_term(), env, *scopes_in_out, backtrace_bind));
        } else if (type.type == TERM_TYPE_STREAM || type.type == TERM_TYPE_VIEW) {
            throw runtime_exc_t("Cannot bind streams/views to variable names",
                                backtrace);
        } else if (type.type == TERM_TYPE_ARBITRARY) {
            eval_term_as_json(let->mutable_binds(i)->mutable_term(), env, *scopes_in_out, backtrace_bind);
            unreachable("This term has type `TERM_TYPE_ARBITRARY`, so "
                "evaluating it must throw  `runtime_exc_t`.");
        }

        scopes_in_out->type_env.scope.put_in_scope(let->binds(i).var(), type);
    }
}

boost::shared_ptr<scoped_cJSON_t> eval_term_as_json_and_check(Term3 *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, int type, const std::string &msg) {
    boost::shared_ptr<scoped_cJSON_t> res = eval_term_as_json(t, env, scopes, backtrace);
    if (res->type() != type) {
        throw runtime_exc_t(msg, backtrace);
    }
    return res;
}

boost::shared_ptr<scoped_cJSON_t> eval_term_as_json_and_check_either(Term3 *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, int type1, int type2, const std::string &msg) {
    boost::shared_ptr<scoped_cJSON_t> res = eval_term_as_json(t, env, scopes, backtrace);
    if (res->type() != type1 && res->type() != type2) {
        throw runtime_exc_t(msg, backtrace);
    }
    return res;
}


boost::shared_ptr<scoped_cJSON_t> eval_term_as_json(Term3 *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    switch (t->type()) {
    case Term3::IMPLICIT_VAR:
        guarantee_debug_throw_release(scopes.implicit_attribute_value.has_value(), backtrace);
        return scopes.implicit_attribute_value.get_value();
    case Term3::VAR: {
        boost::shared_ptr<scoped_cJSON_t> ptr = scopes.scope.get(t->var());
        guarantee_debug_throw_release(ptr.get() && ptr->get(), backtrace);
        return ptr;
    } break;
    case Term3::LET:
        {
            // Push the scope
            scopes_t scopes_copy = scopes;
            new_scope_t inner_type_scope(&scopes_copy.type_env.scope);
            new_val_scope_t inner_val_scope(&scopes_copy.scope);

            eval_let_binds(t->mutable_let(), env, &scopes_copy, backtrace);

            return eval_term_as_json(t->mutable_let()->mutable_expr(), env, scopes_copy, backtrace.with("expr"));
        }
        break;
    case Term3::IF:
        {
            boost::shared_ptr<scoped_cJSON_t> test = eval_term_as_json_and_check_either(t->mutable_if_()->mutable_test(), env, scopes, backtrace.with("test"), cJSON_True, cJSON_False, "The IF test must evaluate to a boolean.");

            boost::shared_ptr<scoped_cJSON_t> res;
            if (test->type() == cJSON_True) {
                res = eval_term_as_json(t->mutable_if_()->mutable_true_branch(), env, scopes, backtrace.with("true"));
            } else {
                res = eval_term_as_json(t->mutable_if_()->mutable_false_branch(), env, scopes, backtrace.with("false"));
            }
            return res;
        }
        break;
    case Term3::ERROR:
        throw runtime_exc_t(t->error(), backtrace);
        break;
    case Term3::NUMBER:
        {
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(safe_cJSON_CreateNumber(t->number(), backtrace)));
        }
        break;
    case Term3::STRING:
        {
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateString(t->valuestring().c_str())));
        }
        break;
    case Term3::JSON: {
        const char *str = t->jsonstring().c_str();
        boost::shared_ptr<scoped_cJSON_t> json(new scoped_cJSON_t(cJSON_Parse(str)));
        if (!json->get()) {
            throw runtime_exc_t(strprintf("Malformed JSON: %s", str), backtrace);
        }
        return json;
    } break;
    case Term3::BOOL:
        {
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateBool(t->valuebool())));
        }
        break;
    case Term3::JSON_NULL:
        {
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNull()));
        }
        break;
    case Term3::ARRAY:
        {
            boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));
            for (int i = 0; i < t->array_size(); ++i) {
                res->AddItemToArray(eval_term_as_json(t->mutable_array(i), env, scopes, backtrace.with(strprintf("elem:%d", i)))->DeepCopy());
            }
            return res;
        }
        break;
    case Term3::OBJECT:
        {
            boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateObject()));
            for (int i = 0; i < t->object_size(); ++i) {
                std::string item_name(t->object(i).var());
                res->AddItemToObject(item_name.c_str(), eval_term_as_json(t->mutable_object(i)->mutable_term(), env, scopes, backtrace.with(strprintf("key:%s", item_name.c_str())))->DeepCopy());
            }
            return res;
        }
        break;
    case Term3::GETBYKEY:
        {
            std::string pk = get_primary_key(t->mutable_get_by_key()->mutable_table_ref(), env, backtrace);

            if (t->get_by_key().attrname() != pk) {
                throw runtime_exc_t(strprintf("Attribute: %s is not the primary key (%s) and thus cannot be selected upon.",
                                              t->get_by_key().attrname().c_str(), pk.c_str()), backtrace.with("attrname"));
            }

            namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval_table_ref(t->mutable_get_by_key()->mutable_table_ref(), env, backtrace);

            boost::shared_ptr<scoped_cJSON_t> key = eval_term_as_json(t->mutable_get_by_key()->mutable_key(), env, scopes, backtrace.with("key"));

            try {
                rdb_protocol_t::point_read_response_t p_res =
                    read_by_key(ns_access, env, key->get(), t->get_by_key().table_ref().use_outdated(), backtrace);
                return p_res.data;
            } catch (cannot_perform_query_exc_t e) {
                throw runtime_exc_t("cannot perform read: " + std::string(e.what()), backtrace);
            }
            break;
        }
    case Term3::TABLE:
        crash("Term3::TABLE must be evaluated with eval_stream or eval_view");

    case Term3::JAVASCRIPT: {
        // TODO(rntz): optimize map/reduce/filter queries whose
        // mappings/reductions/predicates are just javascript terms by
        // streaming arguments to them and streaming results back. Should
        // wait to do this until it's established it's a bottleneck, but
        // it's a reasonably good bet that it is. Putting this comment here
        // not because the changes will need to be made here but because
        // it's the most logical place to put it.

        // TODO (rntz): implicitly bound argument should become receiver
        // ("this") object on javascript side.

        boost::shared_ptr<js::runner_t> js = env->get_js_runner();
        std::string errmsg;
        boost::shared_ptr<scoped_cJSON_t> result;

        // TODO(rntz): set up a js::runner_t::req_config_t with an
        // appropriately-chosen timeout.

        // Check whether the function has been compiled already.
        bool compiled = t->HasExtension(extension::js_id);

        // We give all values in scope as arguments.
        // TODO(rntz): this is wasteful double-copying.
        std::vector<std::string> argnames; // only used if (!compiled)
        std::vector<boost::shared_ptr<scoped_cJSON_t> > argvals;
        scopes.scope.dump(compiled ? NULL : &argnames, &argvals);

        js::id_t id;
        if (compiled) {
            id = t->GetExtension(extension::js_id);
        } else {
            // Not compiled yet. Compile it and add the extension.
            id = js->compile(argnames, t->javascript(), &errmsg);
            if (js::INVALID_ID == id) {
                throw runtime_exc_t("failed to compile javascript: " + errmsg,
                                    backtrace);
            }
            t->SetExtension(extension::js_id, (int32_t) id);
        }

        // Figure out whether to bind "this" to the implicit object.
        boost::shared_ptr<scoped_cJSON_t> object;
        if (scopes.implicit_attribute_value.has_value()) {
            object = scopes.implicit_attribute_value.get_value();
            if (object->type() != cJSON_Object) {
                // If it's not a JSON object, we have to ignore it ("this"
                // can't be bound to a non-object).
                object.reset();
            }
        }

        // Evaluate the source.
        throw runtime_exc_t("failed to evaluate javascript: " + errmsg, backtrace);
        return result;
    }

    default:
        unreachable();
    }
    unreachable();
}

boost::shared_ptr<json_stream_t> eval_term_as_stream(Term3 *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    switch (t->type()) {
    case Term3::LET:
        {
            // Push the scope
            scopes_t scopes_copy = scopes;
            new_scope_t inner_type_scope(&scopes_copy.type_env.scope);
            new_val_scope_t inner_val_scope(&scopes_copy.scope);

            eval_let_binds(t->mutable_let(), env, &scopes_copy, backtrace);

            return eval_term_as_stream(t->mutable_let()->mutable_expr(), env, scopes_copy, backtrace.with("expr"));
        }
        break;
    case Term3::IF:
        {
            boost::shared_ptr<scoped_cJSON_t> test = eval_term_as_json_and_check_either(t->mutable_if_()->mutable_test(), env, scopes, backtrace.with("test"), cJSON_True, cJSON_False, "The IF test must evaluate to a boolean.");

            if (test->type() == cJSON_True) {
                return eval_term_as_stream(t->mutable_if_()->mutable_true_branch(), env, scopes, backtrace.with("true"));
            } else {
                return eval_term_as_stream(t->mutable_if_()->mutable_false_branch(), env, scopes, backtrace.with("false"));
            }
        }
        break;
    case Term3::TABLE:
        {
            return eval_table_as_view(t->mutable_table(), env, backtrace).stream;
        }
        break;
    case Term3::IMPLICIT_VAR:
    case Term3::VAR:
    case Term3::ERROR:
    case Term3::NUMBER:
    case Term3::STRING:
    case Term3::JSON:
    case Term3::BOOL:
    case Term3::JSON_NULL:
    case Term3::OBJECT:
    case Term3::GETBYKEY:
    case Term3::JAVASCRIPT:
    case Term3::ARRAY: {
        boost::shared_ptr<scoped_cJSON_t> arr = eval_term_as_json(t, env, scopes, backtrace);
        require_type(arr->get(), cJSON_Array, backtrace);
        json_array_iterator_t it(arr->get());
        return boost::shared_ptr<json_stream_t>(new in_memory_stream_t(it));
    } break;
    default:
        unreachable();
        break;
    }
    unreachable();
}

predicate_t::predicate_t(const Predicate &_pred, runtime_environment_t *_env, const scopes_t &_scopes, const backtrace_t &_backtrace)
    : pred(_pred), env(_env), scopes(_scopes), backtrace(_backtrace)
{ }

bool predicate_t::operator()(boost::shared_ptr<scoped_cJSON_t> json) {
    variable_val_scope_t::new_scope_t scope_maker(&scopes.scope, pred.arg(), json);
    variable_type_scope_t::new_scope_t type_scope_maker(&scopes.type_env.scope, pred.arg(), term_info_t(TERM_TYPE_JSON, true));

    implicit_value_setter_t impliciter(&scopes.implicit_attribute_value, json);
    implicit_type_t::impliciter_t type_impliciter(&scopes.type_env.implicit_type, term_info_t(TERM_TYPE_JSON, true));

    boost::shared_ptr<scoped_cJSON_t> a_bool = eval_term_as_json(pred.mutable_body(), env, scopes, backtrace);

    if (a_bool->type() == cJSON_True) {
        return true;
    } else if (a_bool->type() == cJSON_False) {
        return false;
    } else {
        throw runtime_exc_t("Predicate failed to evaluate to a bool", backtrace);
    }
}

/* Renaming map here because otherwise it conflicts with std::map. */
boost::shared_ptr<scoped_cJSON_t> map_rdb(std::string arg, Term3 *term, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, boost::shared_ptr<scoped_cJSON_t> val) {
    scopes_t scopes_copy = scopes;

    variable_val_scope_t::new_scope_t scope_maker(&scopes_copy.scope, arg, val);
    variable_type_scope_t::new_scope_t type_scope_maker(&scopes_copy.type_env.scope, arg, term_info_t(TERM_TYPE_JSON, true));

    implicit_value_setter_t impliciter(&scopes_copy.implicit_attribute_value, val);
    implicit_type_t::impliciter_t type_impliciter(&scopes_copy.type_env.implicit_type, term_info_t(TERM_TYPE_JSON, true));
    return eval_term_as_json(term, env, scopes_copy, backtrace);
}

boost::shared_ptr<scoped_cJSON_t> eval_mapping(Mapping m, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace,
                                               boost::shared_ptr<scoped_cJSON_t> val) {
    return map_rdb(m.arg(), m.mutable_body(), env, scopes, backtrace, val);
}

boost::shared_ptr<json_stream_t> concatmap(std::string arg, Term3 *term, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, boost::shared_ptr<scoped_cJSON_t> val) {
    scopes_t scopes_copy = scopes;
    variable_val_scope_t::new_scope_t scope_maker(&scopes_copy.scope, arg, val);
    variable_type_scope_t::new_scope_t type_scope_maker(&scopes_copy.type_env.scope, arg, term_info_t(TERM_TYPE_JSON, true));

    implicit_value_setter_t impliciter(&scopes_copy.implicit_attribute_value, val);
    implicit_type_t::impliciter_t type_impliciter(&scopes_copy.type_env.implicit_type, term_info_t(TERM_TYPE_JSON, true));
    return eval_term_as_stream(term, env, scopes_copy, backtrace);
}

namespace_repo_t<rdb_protocol_t>::access_t eval_table_ref(TableRef *t, runtime_environment_t *env, const backtrace_t &bt)
    THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    name_string_t table_name;
    assign_table_name(t->table_name(), bt.with("table_name"), &table_name);

    name_string_t db_name;
    assign_db_name(t->db_name(), bt.with("db_name"), &db_name);

    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > namespaces_metadata = env->namespaces_semilattice_metadata->get();
    databases_semilattice_metadata_t databases_metadata = env->databases_semilattice_metadata->get();

    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t namespaces_metadata_change(&namespaces_metadata);
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
        ns_searcher(&namespaces_metadata_change.get()->namespaces);
    metadata_searcher_t<database_semilattice_metadata_t>
        db_searcher(&databases_metadata.databases);

    uuid_u db_id = meta_get_uuid(db_searcher, db_name, "EVAL_DB " + db_name.str(), bt);
    namespace_predicate_t pred(&table_name, &db_id);
    uuid_u id = meta_get_uuid(ns_searcher, pred, "EVAL_TABLE " + table_name.str(), bt);

    return namespace_repo_t<rdb_protocol_t>::access_t(
        env->ns_repo, id, env->interruptor);
}

view_t eval_term_as_view(Term3 *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    switch (t->type()) {
    case Term3::IMPLICIT_VAR:
    case Term3::VAR:
    case Term3::LET:
    case Term3::ERROR:
    case Term3::NUMBER:
    case Term3::STRING:
    case Term3::JSON:
    case Term3::BOOL:
    case Term3::JSON_NULL:
    case Term3::ARRAY:
    case Term3::OBJECT:
    case Term3::JAVASCRIPT:
        crash("eval_term_as_view called on incompatible Term3");
        break;
    case Term3::IF:
        {
            boost::shared_ptr<scoped_cJSON_t> test = eval_term_as_json_and_check_either(t->mutable_if_()->mutable_test(), env, scopes, backtrace.with("test"), cJSON_True, cJSON_False, "The IF test must evaluate to a boolean.");

            if (test->type() == cJSON_True) {
                return eval_term_as_view(t->mutable_if_()->mutable_true_branch(), env, scopes, backtrace.with("true"));
            } else {
                return eval_term_as_view(t->mutable_if_()->mutable_false_branch(), env, scopes, backtrace.with("false"));
            }
        }
        break;
    case Term3::GETBYKEY:
        crash("unimplemented");
        break;
    case Term3::TABLE:
        return eval_table_as_view(t->mutable_table(), env, backtrace.with("table_ref"));
        break;
    default:
        unreachable();
    }
    unreachable();
}

view_t eval_table_as_view(Term3::Table *t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval_table_ref(t->mutable_table_ref(), env, backtrace);
    std::string pk = get_primary_key(t->mutable_table_ref(), env, backtrace);
    // TODO(jdoliner): We used to pass 100 as a batch_size parameter to the batched_rget_stream_t constructor.
    boost::shared_ptr<json_stream_t> stream(new batched_rget_stream_t(ns_access, env->interruptor, key_range_t::universe(), backtrace, t->table_ref().use_outdated()));
    return view_t(ns_access, pk, stream);
}

} //namespace query_language

#undef guarantee_debug_throw_release
