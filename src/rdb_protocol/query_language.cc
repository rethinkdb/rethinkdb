#include "rdb_protocol/query_language.hpp"

#include <math.h>

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <boost/variant.hpp>

#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/suggester.hpp"
#include "http/json.hpp"
#include "rdb_protocol/internal_extensions.pb.h"
#include "rdb_protocol/js.hpp"
#include "rpc/directory/read_manager.hpp"
#include "rdb_protocol/proto_utils.hpp"


//TODO: why is this not in the query_language namespace?
void wait_for_rdb_table_readiness(namespace_repo_t<rdb_protocol_t> *ns_repo, namespace_id_t namespace_id, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    /* The following is an ugly hack, but it's probably what we want.  It
       takes about a third of a second for the new namespace to get to the
       point where we can do reads/writes on it.  We don't want to return
       until that has happened, so we try to do a read every `poll_ms`
       milliseconds until one succeeds, then return. */

    // This read won't succeed, but we care whether it fails with an exception.
    // It must be an rget to make sure that access is available to all shards.

    //TODO: lower this for release?
    const int poll_ms = 100; //with this value, usually polls twice
    //TODO: why is this still named bad*_read?  It looks like a valid read to me.
    rdb_protocol_t::rget_read_t bad_rget_read(hash_region_t<key_range_t>::universe());
    rdb_protocol_t::read_t bad_read(bad_rget_read);
    for (;;) {
        signal_timer_t start_poll(poll_ms);
        wait_interruptible(&start_poll, interruptor);
        try {
            namespace_repo_t<rdb_protocol_t>::access_t ns_access(ns_repo, namespace_id, interruptor);
            rdb_protocol_t::read_response_t read_res;
            // TODO: We should not use order_token_t::ignore.
            ns_access.get_namespace_if()->read(bad_read, &read_res, order_token_t::ignore, interruptor);
            break;
        } catch (cannot_perform_query_exc_t e) { } //continue loop
    }
}

namespace query_language {

cJSON *safe_cJSON_CreateNumber(double d, const backtrace_t &backtrace) {
    if (!isfinite(d)) throw runtime_exc_t(strprintf("Illegal numeric value %e.", d), backtrace);
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

term_info_t get_term_type(Term *t, type_checking_environment_t *env, const backtrace_t &backtrace) {
    if (t->HasExtension(extension::inferred_type)) {
        guarantee_debug_throw_release(t->HasExtension(extension::deterministic), backtrace);
        int type = t->GetExtension(extension::inferred_type);
        bool det = t->GetExtension(extension::deterministic);
        return term_info_t(static_cast<term_type_t>(type), det);
    }
    check_protobuf(Term::TermType_IsValid(t->type()));

    std::vector<const google::protobuf::FieldDescriptor *> fields;
    t->GetReflection()->ListFields(*t, &fields);
    int field_count = fields.size();

    check_protobuf(field_count <= 2);

    boost::optional<term_info_t> ret;
    switch (t->type()) {
    case Term::IMPLICIT_VAR: {
        if (!env->implicit_type.has_value() || env->implicit_type.get_value().type != TERM_TYPE_JSON) {
            throw bad_query_exc_t("No implicit variable in scope", backtrace);
        } else if (!env->implicit_type.depth_is_legal()) {
            throw bad_query_exc_t("Cannot use implicit variable in nested queries.  Name your variables!", backtrace);
        }
        ret.reset(env->implicit_type.get_value());
    } break;
    case Term::VAR: {
        check_protobuf(t->has_var());
        if (!env->scope.is_in_scope(t->var())) {
            throw bad_query_exc_t(strprintf("symbol '%s' is not in scope", t->var().c_str()), backtrace);
        }
        ret.reset(env->scope.get(t->var()));
    } break;
    case Term::LET: {
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
    case Term::CALL: {
        check_protobuf(t->has_call());
        ret.reset(get_function_type(t->mutable_call(), env, backtrace));
    } break;
    case Term::IF: {
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
    case Term::ERROR: {
        check_protobuf(t->has_error());
        ret.reset(term_info_t(TERM_TYPE_ARBITRARY, true));
    } break;
    case Term::NUMBER: {
        check_protobuf(t->has_number());
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term::STRING: {
        check_protobuf(t->has_valuestring());
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term::JSON: {
        check_protobuf(t->has_jsonstring());
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term::BOOL: {
        check_protobuf(t->has_valuebool());
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term::JSON_NULL: {
        check_protobuf(field_count == 1); // null term has only a type field
        ret.reset(term_info_t(TERM_TYPE_JSON, true));
    } break;
    case Term::ARRAY: {
        if (t->array_size() == 0) { // empty arrays are valid
            check_protobuf(field_count == 1);
        }
        bool deterministic = true;
        for (int i = 0; i < t->array_size(); ++i) {
            check_term_type(t->mutable_array(i), TERM_TYPE_JSON, env, &deterministic, backtrace.with(strprintf("elem:%d", i)));
        }
        ret.reset(term_info_t(TERM_TYPE_JSON, deterministic));
    } break;
    case Term::OBJECT: {
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
    case Term::GETBYKEY: {
        check_protobuf(t->has_get_by_key());
        check_term_type(t->mutable_get_by_key()->mutable_key(), TERM_TYPE_JSON, env, NULL, backtrace.with("key"));
        ret.reset(term_info_t(TERM_TYPE_JSON, false));
    } break;
    case Term::TABLE: {
        check_protobuf(t->has_table());
        ret.reset(term_info_t(TERM_TYPE_VIEW, false));
    } break;
    case Term::JAVASCRIPT: {
        check_protobuf(t->has_javascript());
        ret.reset(term_info_t(TERM_TYPE_JSON, false)); //javascript is never deterministic
    } break;
    default: unreachable("unhandled Term case");
    }
    guarantee_debug_throw_release(ret, backtrace);
    t->SetExtension(extension::inferred_type, static_cast<int32_t>(ret->type));
    t->SetExtension(extension::deterministic, ret->deterministic);
    //debugf("%s", t->DebugString().c_str());
    return *ret;
}

void check_term_type(Term *t, term_type_t expected, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
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

void check_arg_count(Term::Call *c, int n_args, const backtrace_t &backtrace) {
    if (c->args_size() != n_args) {
        const char* fn_name = Builtin::BuiltinType_Name(c->builtin().type()).c_str();
        throw bad_query_exc_t(strprintf(
            "%s takes %d argument%s (%d given)",
            fn_name,
            n_args,
            n_args != 1 ? "s" : "",
            c->args_size()
            ),
            backtrace);
    }
}

void check_arg_count_at_least(Term::Call *c, int n_args, const backtrace_t &backtrace) {
    if (c->args_size() < n_args) {
        const char* fn_name = Builtin::BuiltinType_Name(c->builtin().type()).c_str();
        throw bad_query_exc_t(strprintf(
            "%s takes at least %d argument%s (%d given)",
            fn_name,
            n_args,
            n_args != 1 ? "s" : "",
            c->args_size()
            ),
            backtrace);
    }
}

void check_function_args(Term::Call *c, const term_type_t &arg_type, int n_args,
                         type_checking_environment_t *env, bool *is_det_out,
                         const backtrace_t &backtrace) {
    check_arg_count(c, n_args, backtrace);
    for (int i = 0; i < c->args_size(); ++i) {
        check_term_type(c->mutable_args(i), arg_type, env, is_det_out, backtrace.with(strprintf("arg:%d", i)));
    }
}

void check_function_args_at_least(Term::Call *c, const term_type_t &arg_type, int n_args,
                                  type_checking_environment_t *env, bool *is_det_out,
                                  const backtrace_t &backtrace) {
    // requires the number of arguments to be at least n_args
    check_arg_count_at_least(c, n_args, backtrace);
    for (int i = 0; i < c->args_size(); ++i) {
        check_term_type(c->mutable_args(i), arg_type, env, is_det_out, backtrace.with(strprintf("arg:%d", i)));
    }
}

void check_polymorphic_function_args(Term::Call *c, const term_type_t &arg_type, int n_args,
                         type_checking_environment_t *env, bool *is_det_out,
                         const backtrace_t &backtrace) {
    // for functions that work with selections/streams/jsons, where the first argument can be any type
    check_arg_count(c, n_args, backtrace);
    for (int i = 1; i < c->args_size(); ++i) {
        check_term_type(c->mutable_args(i), arg_type, env, is_det_out, backtrace.with(strprintf("arg:%d", i)));
    }
}

void check_function_args(Term::Call *c, const term_type_t &arg1_type, const term_type_t &arg2_type,
                         type_checking_environment_t *env, bool *is_det_out,
                         const backtrace_t &backtrace) {
    check_arg_count(c, 2, backtrace);
    check_term_type(c->mutable_args(0), arg1_type, env, is_det_out, backtrace.with("arg:0"));
    check_term_type(c->mutable_args(1), arg2_type, env, is_det_out, backtrace.with("arg:1"));
}

term_info_t get_function_type(Term::Call *c, type_checking_environment_t *env, const backtrace_t &backtrace) {
    Builtin *b = c->mutable_builtin();

    check_protobuf(Builtin::BuiltinType_IsValid(b->type()));

    bool deterministic = true;
    std::vector<const google::protobuf::FieldDescriptor *> fields;


    b->GetReflection()->ListFields(*b, &fields);

    int field_count = fields.size();

    check_protobuf(field_count <= 2);

    // this is a bit cleaner when we check well-formedness separate
    // from returning the type

    switch (c->mutable_builtin()->type()) {
    case Builtin::NOT:
    case Builtin::MAPMERGE:
    case Builtin::ARRAYAPPEND:
    case Builtin::ADD:
    case Builtin::SUBTRACT:
    case Builtin::MULTIPLY:
    case Builtin::DIVIDE:
    case Builtin::MODULO:
    case Builtin::DISTINCT:
    case Builtin::LENGTH:
    case Builtin::UNION:
    case Builtin::NTH:
    case Builtin::STREAMTOARRAY:
    case Builtin::ARRAYTOSTREAM:
    case Builtin::ANY:
    case Builtin::ALL:
    case Builtin::SLICE:
        // these builtins only have
        // Builtin.type set
        check_protobuf(field_count == 1);
        break;
    case Builtin::COMPARE:
        check_protobuf(b->has_comparison());
        check_protobuf(Builtin::Comparison_IsValid(b->comparison()));
        break;
    case Builtin::GETATTR:
    case Builtin::IMPLICIT_GETATTR:
    case Builtin::HASATTR:
    case Builtin::IMPLICIT_HASATTR:
        check_protobuf(b->has_attr());
        break;
    case Builtin::WITHOUT:
    case Builtin::IMPLICIT_WITHOUT:
    case Builtin::PICKATTRS:
    case Builtin::IMPLICIT_PICKATTRS:
        break;
    case Builtin::FILTER: {
        check_protobuf(b->has_filter());
        break;
    }
    case Builtin::MAP: {
        check_protobuf(b->has_map());
        break;
    }
    case Builtin::CONCATMAP: {
        check_protobuf(b->has_concat_map());
        break;
    }
    case Builtin::ORDERBY:
        check_protobuf(b->order_by_size() > 0);
        break;
    case Builtin::REDUCE: {
        check_protobuf(b->has_reduce());
        break;
    }
    case Builtin::GROUPEDMAPREDUCE: {
        check_protobuf(b->has_grouped_map_reduce());
        break;
    }
    case Builtin::RANGE:
        check_protobuf(b->has_range());
        break;
    default:
        crash("unreachable");
    }

    switch (b->type()) {
        //JSON -> JSON
        case Builtin::NOT:
        case Builtin::GETATTR:
        case Builtin::HASATTR:
        case Builtin::PICKATTRS:
        case Builtin::WITHOUT:
            check_function_args(c, TERM_TYPE_JSON, 1, env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_JSON, deterministic);
            break;
        case Builtin::IMPLICIT_GETATTR:
        case Builtin::IMPLICIT_HASATTR:
        case Builtin::IMPLICIT_PICKATTRS:
        case Builtin::IMPLICIT_WITHOUT:
            check_arg_count(c, 0, backtrace);
            if (!env->implicit_type.has_value() || env->implicit_type.get_value().type != TERM_TYPE_JSON) {
                throw bad_query_exc_t("No implicit variable in scope", backtrace);
            } else if (!env->implicit_type.depth_is_legal()) {
                throw bad_query_exc_t("Cannot use implicit variable in nested queries.  Name your variables!", backtrace);
            }
            deterministic &= env->implicit_type.get_value().deterministic;
            return term_info_t(TERM_TYPE_JSON, deterministic);
            break;
        case Builtin::MAPMERGE:
        case Builtin::ARRAYAPPEND:
        case Builtin::MODULO:
            check_function_args(c, TERM_TYPE_JSON, 2, env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_JSON, deterministic);
            break;
        case Builtin::COMPARE:
            check_function_args_at_least(c, TERM_TYPE_JSON, 1, env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_JSON, deterministic);
            break;
        case Builtin::ADD:
        case Builtin::SUBTRACT:
        case Builtin::MULTIPLY:
        case Builtin::DIVIDE:
        case Builtin::ANY:
        case Builtin::ALL:
            check_function_args(c, TERM_TYPE_JSON, c->args_size(), env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_JSON, deterministic);  // variadic JSON type
            break;
        case Builtin::SLICE:
            {
                check_polymorphic_function_args(c, TERM_TYPE_JSON, 3, env, &deterministic, backtrace);
                // polymorphic
                term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
                res.deterministic &= deterministic;
                return res;
            }
        case Builtin::MAP:
            {
                check_arg_count(c, 1, backtrace);

                {
                    implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                    check_mapping_type(b->mutable_map()->mutable_mapping(), TERM_TYPE_JSON, env, &deterministic, backtrace.with("mapping"));
                }
                term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
                res.deterministic &= deterministic;
                return res;
            }
            break;
        case Builtin::CONCATMAP:
            {
                check_arg_count(c, 1, backtrace);

                {
                    implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                    check_mapping_type(b->mutable_concat_map()->mutable_mapping(), TERM_TYPE_ARBITRARY, env, &deterministic, backtrace.with("mapping"));
                }
                term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
                res.deterministic &= deterministic;
                return res;
            }
            break;
        case Builtin::DISTINCT:
            {
                check_arg_count(c, 1, backtrace);

                term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
                res.deterministic &= deterministic;
                return res;
            }
            break;
        case Builtin::FILTER:
            {
                check_arg_count(c, 1, backtrace);
                {
                    // polymorphic
                    implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                    check_predicate_type(b->mutable_filter()->mutable_predicate(), env, &deterministic, backtrace.with("predicate"));
                }
                term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
                res.deterministic &= deterministic;
                return res;
            }
            break;
        case Builtin::ORDERBY:
            {
                check_arg_count(c, 1, backtrace);

                term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
                res.deterministic &= deterministic;
                return res;
            }
            break;
        case Builtin::NTH:
            {
                check_polymorphic_function_args(c, TERM_TYPE_JSON, 2, env, &deterministic, backtrace);
                term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
                return term_info_t(TERM_TYPE_JSON, deterministic && res.deterministic);
                break;
            }
            break;
        case Builtin::LENGTH:
            {
                check_arg_count(c, 1, backtrace);
                term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
                return term_info_t(TERM_TYPE_JSON, deterministic & res.deterministic);
            }
        case Builtin::STREAMTOARRAY:
            check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_JSON, deterministic);
            break;
        case Builtin::REDUCE:
            {
                check_arg_count(c, 1, backtrace);
                check_reduction_type(b->mutable_reduce(), env, &deterministic, backtrace.with("reduce"));
                term_info_t argtype = get_term_type(c->mutable_args(0), env, backtrace);
                return term_info_t(TERM_TYPE_JSON, deterministic && argtype.deterministic);
            }
            break;
        case Builtin::GROUPEDMAPREDUCE:
            {
                check_arg_count(c, 1, backtrace);
                {
                    check_mapping_type(b->mutable_grouped_map_reduce()->mutable_group_mapping(), TERM_TYPE_JSON, env, &deterministic, backtrace.with("group_mapping"));
                    check_mapping_type(b->mutable_grouped_map_reduce()->mutable_value_mapping(), TERM_TYPE_JSON, env, &deterministic, backtrace.with("value_mapping"));
                    check_reduction_type(b->mutable_grouped_map_reduce()->mutable_reduction(), env, &deterministic, backtrace.with("reduction"));
                }
                term_info_t argtype = get_term_type(c->mutable_args(0), env, backtrace);
                return term_info_t(TERM_TYPE_JSON, deterministic && argtype.deterministic);
            }
            break;
        case Builtin::UNION: {
            check_arg_count(c, 2, backtrace);
            term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
            check_function_args(c, res.type, 2, env, &deterministic, backtrace);
            res.deterministic &= deterministic;
            return res;
        } break;
        case Builtin::ARRAYTOSTREAM: {
            check_function_args(c, TERM_TYPE_JSON, 1, env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_STREAM, deterministic);
            break;
        } break;
        case Builtin::RANGE: {
            if (b->range().has_lowerbound()) {
                check_term_type(b->mutable_range()->mutable_lowerbound(), TERM_TYPE_JSON, env, &deterministic, backtrace.with("lowerbound"));
            }
            if (b->range().has_upperbound()) {
                check_term_type(b->mutable_range()->mutable_upperbound(), TERM_TYPE_JSON, env, &deterministic, backtrace.with("upperbound"));
            }
            check_arg_count(c, 1, backtrace);
            term_info_t res = get_term_type(c->mutable_args(0), env, backtrace);
            res.deterministic &= deterministic;
            return res;
        } break;
        default:
            crash("unreachable");
            break;
    }

    crash("unreachable");
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
    env->scope.put_in_scope(m->arg(), term_info_t(TERM_TYPE_JSON, true/*deterministic*/));
    check_term_type(m->mutable_body(), return_type, env, &is_det, backtrace);
    *is_det_out &= is_det;
}

void check_predicate_type(Predicate *p, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(p->arg(), term_info_t(TERM_TYPE_JSON, true));
    check_term_type(p->mutable_body(), TERM_TYPE_JSON, env, is_det_out, backtrace);
}

void check_read_query_type(ReadQuery *rq, type_checking_environment_t *env, bool *, const backtrace_t &backtrace) {
    /* Read queries could return anything--a view, a stream, a JSON, or an
    error. Views will be automatically converted to streams at evaluation time.
    */
    term_info_t res = get_term_type(rq->mutable_term(), env, backtrace);
    rq->SetExtension(extension::inferred_read_type, static_cast<int32_t>(res.type));
}

void check_write_query_type(WriteQuery *w, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    check_protobuf(WriteQuery::WriteQueryType_IsValid(w->type()));

    std::vector<const google::protobuf::FieldDescriptor *> fields;
    w->GetReflection()->ListFields(*w, &fields);
    check_protobuf(static_cast<int64_t>(fields.size()) == 2 + w->has_atomic());

    bool deterministic = true;
    switch (w->type()) {
    case WriteQuery::UPDATE: {
        check_protobuf(w->has_update());
        check_term_type(w->mutable_update()->mutable_view(), TERM_TYPE_VIEW, env, is_det_out, backtrace.with("view"));
        implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic));
        check_mapping_type(w->mutable_update()->mutable_mapping(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("modify_map"));
    } break;
    case WriteQuery::DELETE: {
        check_protobuf(w->has_delete_());
        check_term_type(w->mutable_delete_()->mutable_view(), TERM_TYPE_VIEW, env, is_det_out, backtrace.with("view"));
    } break;
    case WriteQuery::MUTATE: {
        check_protobuf(w->has_mutate());
        check_term_type(w->mutable_mutate()->mutable_view(), TERM_TYPE_VIEW, env, is_det_out, backtrace.with("view"));
        implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic));
        check_mapping_type(w->mutable_mutate()->mutable_mapping(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("modify_map"));
    } break;
    case WriteQuery::INSERT: {
        check_protobuf(w->has_insert());
        if (w->insert().terms_size() == 1) {
            // We want to do the get to produce determinism information
            get_term_type(w->mutable_insert()->mutable_terms(0), env, backtrace);
            break; //Single-element insert polymorphic over streams and arrays
        }
        for (int i = 0; i < w->insert().terms_size(); ++i) {
            check_term_type(w->mutable_insert()->mutable_terms(i), TERM_TYPE_JSON, env, is_det_out, backtrace.with(strprintf("term:%d", i)));
        }
    } break;
    case WriteQuery::FOREACH: {
        check_protobuf(w->has_for_each());
        check_term_type(w->mutable_for_each()->mutable_stream(), TERM_TYPE_ARBITRARY, env, is_det_out, backtrace.with("stream"));

        new_scope_t scope_maker(&env->scope, w->mutable_for_each()->var(), term_info_t(TERM_TYPE_JSON, true));
        for (int i = 0; i < w->for_each().queries_size(); ++i) {
            check_write_query_type(w->mutable_for_each()->mutable_queries(i), env, is_det_out, backtrace.with(strprintf("query:%d", i)));
        }
    } break;
    case WriteQuery::POINTUPDATE: {
        check_protobuf(w->has_point_update());
        check_term_type(w->mutable_point_update()->mutable_key(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("key"));
        implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic));
        check_mapping_type(w->mutable_point_update()->mutable_mapping(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("point_map"));
    } break;
    case WriteQuery::POINTDELETE: {
        check_protobuf(w->has_point_delete());
        check_term_type(w->mutable_point_delete()->mutable_key(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("key"));
    } break;
    case WriteQuery::POINTMUTATE: {
        check_protobuf(w->has_point_mutate());
        check_term_type(w->mutable_point_mutate()->mutable_key(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("key"));
        implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic));
        check_mapping_type(w->mutable_point_mutate()->mutable_mapping(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("point_map"));
    } break;
    default:
        unreachable("unhandled WriteQuery");
    }
}

void check_meta_query_type(MetaQuery *t) {
    check_protobuf(MetaQuery::MetaQueryType_IsValid(t->type()));
    switch(t->type()) {
    case MetaQuery::CREATE_DB:
        check_protobuf(t->has_db_name());
        check_protobuf(!t->has_create_table());
        check_protobuf(!t->has_drop_table());
        break;
    case MetaQuery::DROP_DB:
        check_protobuf(t->has_db_name());
        check_protobuf(!t->has_create_table());
        check_protobuf(!t->has_drop_table());
        break;
    case MetaQuery::LIST_DBS:
        check_protobuf(!t->has_db_name());
        check_protobuf(!t->has_create_table());
        check_protobuf(!t->has_drop_table());
        break;
    case MetaQuery::CREATE_TABLE:
        check_protobuf(!t->has_db_name());
        check_protobuf(t->has_create_table());
        check_protobuf(!t->has_drop_table());
        break;
    case MetaQuery::DROP_TABLE:
        check_protobuf(!t->has_db_name());
        check_protobuf(!t->has_create_table());
        check_protobuf(t->has_drop_table());
        break;
    case MetaQuery::LIST_TABLES:
        check_protobuf(t->has_db_name());
        check_protobuf(!t->has_create_table());
        check_protobuf(!t->has_drop_table());
        break;
    default: unreachable("Unhandled MetaQuery.");
    }
}

void check_query_type(Query *q, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    check_protobuf(Query::QueryType_IsValid(q->type()));
    switch (q->type()) {
    case Query::READ:
        check_protobuf(q->has_read_query());
        check_protobuf(!q->has_write_query());
        check_protobuf(!q->has_meta_query());
        check_read_query_type(q->mutable_read_query(), env, is_det_out, backtrace);
        break;
    case Query::WRITE:
        check_protobuf(q->has_write_query());
        check_protobuf(!q->has_read_query());
        check_protobuf(!q->has_meta_query());
        check_write_query_type(q->mutable_write_query(), env, is_det_out, backtrace);
        break;
    case Query::CONTINUE:
        check_protobuf(!q->has_read_query());
        check_protobuf(!q->has_write_query());
        check_protobuf(!q->has_meta_query());
        break;
    case Query::STOP:
        check_protobuf(!q->has_read_query());
        check_protobuf(!q->has_write_query());
        check_protobuf(!q->has_meta_query());
        break;
    case Query::META:
        check_protobuf(q->has_meta_query());
        check_protobuf(!q->has_read_query());
        check_protobuf(!q->has_write_query());
        check_meta_query_type(q->mutable_meta_query());
        break;
    default:
        unreachable("unhandled Query");
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
uuid_t meta_get_uuid(T searcher, const U& predicate,
                     const std::string &operation, const backtrace_t &backtrace) {
    metadata_search_status_t status;
    typename T::iterator entry = searcher.find_uniq(predicate, &status);
    meta_check(status, METADATA_SUCCESS, operation, backtrace);
    return entry->first;
}

void execute_meta(MetaQuery *m, runtime_environment_t *env, Response *res, const backtrace_t &bt) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
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

    switch(m->type()) {
    case MetaQuery::CREATE_DB: {
        std::string db_name = m->db_name();

        /* Ensure database doesn't already exist. */
        metadata_search_status_t status;
        db_searcher.find_uniq(db_name, &status);
        meta_check(status, METADATA_ERR_NONE, "CREATE_DB " + db_name, bt);

        /* Create namespace, insert into metadata, then join into real metadata. */
        database_semilattice_metadata_t db;
        db.name = vclock_t<std::string>(db_name, env->this_machine);
        metadata.databases.databases.insert(std::make_pair(generate_uuid(), db));
        try {
            fill_in_blueprints(&metadata, directory_metadata->get(), env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            throw runtime_exc_t(e.what(), bt);
        }
        env->semilattice_metadata->join(metadata);
        res->set_status_code(Response::SUCCESS_EMPTY); //return immediately.
    } break;
    case MetaQuery::DROP_DB: {
        std::string db_name = m->db_name();

        // Get database metadata.
        metadata_search_status_t status;
        metadata_searcher_t<database_semilattice_metadata_t>::iterator
            db_metadata = db_searcher.find_uniq(db_name, &status);
        meta_check(status, METADATA_SUCCESS, "DROP_DB " + db_name, bt);
        guarantee(!db_metadata->second.is_deleted());
        uuid_t db_id = db_metadata->first;

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
        //TODO: (!!!) catch machine_missing_exc_t (or something)
        try {
            fill_in_blueprints(&metadata, directory_metadata->get(), env->this_machine, false);
        } catch (const missing_machine_exc_t &e) {
            throw runtime_exc_t(e.what(), bt);
        }
        env->semilattice_metadata->join(metadata);
        res->set_status_code(Response::SUCCESS_EMPTY);
    } break;
    case MetaQuery::LIST_DBS: {
        for (metadata_searcher_t<database_semilattice_metadata_t>::iterator
                 it = db_searcher.find_next(db_searcher.begin());
             it != db_searcher.end(); it = db_searcher.find_next(++it)) {

            // For each undeleted and unconflicted entry in the metadata, add a response.
            guarantee(!it->second.is_deleted());
            if (it->second.get().name.in_conflict()) continue;
            scoped_cJSON_t json(cJSON_CreateString(it->second.get().name.get().c_str()));
            res->add_response(json.PrintUnformatted());
        }
        res->set_status_code(Response::SUCCESS_STREAM);
    } break;
    case MetaQuery::CREATE_TABLE: {
        std::string dc_name = m->create_table().datacenter();
        std::string db_name = m->create_table().table_ref().db_name();
        // TODO(1253) do this here or push it up to the create_table() object?
        std::string table_name_str = m->create_table().table_ref().table_name();
        name_string_t table_name;
        if (!table_name.assign(table_name_str)) {
            throw runtime_exc_t("table name invalid (use A-Za-z0-9_): " + table_name_str /* TODO(1253) duplicate message, DRY */, bt);
        }
        std::string primary_key = m->create_table().primary_key();
        int64_t cache_size = m->create_table().cache_size();

        uuid_t db_id = meta_get_uuid(db_searcher, db_name, "FIND_DATABASE " + db_name, bt.with("table_ref").with("db_name"));

        uuid_t dc_id = nil_uuid();
        if (m->create_table().has_datacenter()) {
            dc_id = meta_get_uuid(dc_searcher, dc_name, "FIND_DATACENTER " + dc_name, bt.with("datacenter"));
        }

        namespace_predicate_t search_predicate(&table_name, &db_id);
        /* Ensure table doesn't already exist. */
        metadata_search_status_t status;
        ns_searcher.find_uniq(search_predicate, &status);
        meta_check(status, METADATA_ERR_NONE, "CREATE_TABLE " + table_name.str(), bt);

        /* Create namespace, insert into metadata, then join into real metadata. */
        uuid_t namespace_id = generate_uuid();
        //TODO(mlucy): What is the port for?  Why is it always the same?
        namespace_semilattice_metadata_t<rdb_protocol_t> ns =
            new_namespace<rdb_protocol_t>(env->this_machine, db_id, dc_id, table_name,
                                          primary_key, port_defaults::reql_port, cache_size);
        {
            cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t change(&metadata.rdb_namespaces);
            change.get()->namespaces.insert(std::make_pair(namespace_id, ns));
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
            wait_for_rdb_table_readiness(env->ns_repo, namespace_id, env->interruptor);
        } catch (interrupted_exc_t e) {
            throw runtime_exc_t("Query interrupted, probably by user.", bt);
        }
        res->set_status_code(Response::SUCCESS_EMPTY);
    } break;
    case MetaQuery::DROP_TABLE: {
        std::string db_name = m->drop_table().db_name();
        // TODO(1253): Push name_string_t up to the drop_table object or what?
        std::string table_name_str = m->drop_table().table_name();
        name_string_t table_name;
        if (!table_name.assign(table_name_str)) {
            throw runtime_exc_t("table name invalid (use A-Za-z0-9_): " + table_name_str /* TODO(1253) duplicate message, DRY */, bt);
        }

        // Get namespace metadata.
        uuid_t db_id = meta_get_uuid(db_searcher, db_name, "FIND_DATABASE " + db_name, bt.with("db_name"));
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
        res->set_status_code(Response::SUCCESS_EMPTY); //return immediately
    } break;
    case MetaQuery::LIST_TABLES: {
        std::string db_name = m->db_name();
        uuid_t db_id = meta_get_uuid(db_searcher, db_name, "FIND_DATABASE " + db_name, bt.with("db_name"));
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
        res->set_status_code(Response::SUCCESS_STREAM);
    } break;
    default: crash("unreachable");
    }
}

std::string get_primary_key(TableRef *t, runtime_environment_t *env,
                            const backtrace_t &bt) {
    std::string db_name = t->db_name();
    std::string table_name_str = t->table_name();
    // TODO(1253): push name_string_t up to t?
    name_string_t table_name;
    if (!table_name.assign(table_name_str)) {
        throw runtime_exc_t("invalid table name: " + table_name_str /* TODO(1253) duplicate message, DRY */, bt);
    }

    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > ns_metadata = env->namespaces_semilattice_metadata->get();
    databases_semilattice_metadata_t db_metadata = env->databases_semilattice_metadata->get();

    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t ns_metadata_change(&ns_metadata);
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
        ns_searcher(&ns_metadata_change.get()->namespaces);
    metadata_searcher_t<database_semilattice_metadata_t>
        db_searcher(&db_metadata.databases);

    uuid_t db_id = meta_get_uuid(db_searcher, db_name, "FIND_DB " + db_name, bt);
    namespace_predicate_t pred(&table_name, &db_id);
    metadata_search_status_t status;
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >::iterator
        ns_metadata_it = ns_searcher.find_uniq(pred, &status);
    meta_check(status, METADATA_SUCCESS, "FIND_TABLE " + table_name_str, bt);
    guarantee(!ns_metadata_it->second.is_deleted());
    if (ns_metadata_it->second.get().primary_key.in_conflict()) {
        throw runtime_exc_t(strprintf(
            "Table %s.%s has a primary key in conflict, which should never happen.",
            db_name.c_str(), table_name.c_str()), bt);
    }
    return ns_metadata_it->second.get().primary_key.get();
}

void execute_query(Query *q, runtime_environment_t *env, Response *res, const scopes_t &scopes, const backtrace_t &backtrace, stream_cache_t *stream_cache) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    guarantee_debug_throw_release(q->token() == res->token(), backtrace);
    switch(q->type()) {
    case Query::READ: {
        execute_read_query(q->mutable_read_query(), env, res, scopes, backtrace, stream_cache);
    } break; //status set in [execute_read_query]
    case Query::WRITE: {
        execute_write_query(q->mutable_write_query(), env, res, scopes, backtrace);
    } break; //status set in [execute_write_query]
    case Query::CONTINUE: {
        if (!stream_cache->serve(q->token(), res, env->interruptor)) {
            throw runtime_exc_t(strprintf("Could not serve key %ld from stream cache.", q->token()), backtrace);
        }
    } break; //status set in [serve]
    case Query::STOP: {
        if (!stream_cache->contains(q->token())) {
            throw broken_client_exc_t(strprintf("No key %ld in stream cache.", q->token()));
        } else {
            res->set_status_code(Response::SUCCESS_EMPTY);
            stream_cache->erase(q->token());
        }
    } break;
    case Query::META: {
        execute_meta(q->mutable_meta_query(), env, res, backtrace);
    } break; //status set in [execute_meta]
    default:
        crash("unreachable");
    }
}

void execute_read_query(ReadQuery *r, runtime_environment_t *env, Response *res, const scopes_t &scopes, const backtrace_t &backtrace, stream_cache_t *stream_cache) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    int type = r->GetExtension(extension::inferred_read_type);

    switch (type) {
    case TERM_TYPE_JSON: {
        boost::shared_ptr<scoped_cJSON_t> json = eval_term_as_json(r->mutable_term(), env, scopes, backtrace);
        res->add_response(json->PrintUnformatted());
        res->set_status_code(Response::SUCCESS_JSON);
        break;
    }
    case TERM_TYPE_STREAM:
    case TERM_TYPE_VIEW: {
        boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(r->mutable_term(), env, scopes, backtrace);
        int64_t key = res->token();
        if (stream_cache->contains(key)) {
            throw runtime_exc_t(strprintf("Token %ld already in stream cache, use CONTINUE.", key), backtrace);
        } else {
            stream_cache->insert(r, key, stream);
        }
        bool b = stream_cache->serve(key, res, env->interruptor);
        guarantee_debug_throw_release(b, backtrace);
        break; //status code set in [serve]
    }
    case TERM_TYPE_ARBITRARY: {
        eval_term_as_json(r->mutable_term(), env, scopes, backtrace);
        unreachable("This term has type `TERM_TYPE_ARBITRARY`, so evaluating "
            "it should throw `runtime_exc_t`.");
    }
    default:
        unreachable("read query type invalid.");
    }
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

point_modify::result_t calculate_modify(boost::shared_ptr<scoped_cJSON_t> lhs, const std::string &primary_key, point_modify::op_t op,
                                        const Mapping &mapping, runtime_environment_t *env, const scopes_t &scopes,
                                        const backtrace_t &backtrace, boost::shared_ptr<scoped_cJSON_t> *json_out,
                                        std::string *new_key_out) THROWS_ONLY(runtime_exc_t) {
    guarantee(lhs);
    //CASE 1: UPDATE skips nonexistant entries
    if (lhs->type() == cJSON_NULL && op == point_modify::UPDATE) return point_modify::SKIPPED;
    guarantee_debug_throw_release((lhs->type() == cJSON_NULL && op == point_modify::MUTATE)
                                  || lhs->type() == cJSON_Object, backtrace);

    // Evaluate the mapping.
    boost::shared_ptr<scoped_cJSON_t> rhs = eval_mapping(mapping, env, scopes, backtrace, lhs);
    guarantee_debug_throw_release(rhs, backtrace);

    if (rhs->type() == cJSON_NULL) { //CASE 2: if [rhs] is NULL, we either SKIP or DELETE (or NOP if already deleted)
        if (op == point_modify::UPDATE) return point_modify::SKIPPED;
        guarantee(op == point_modify::MUTATE);
        return (lhs->type() == cJSON_NULL) ? point_modify::NOP : point_modify::DELETED;
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
                                      point_modify::op_name(op).c_str(), primary_key.c_str(),
                                      lhs->Print().c_str(), rhs->Print().c_str()), backtrace);
    }

    if (op == point_modify::MUTATE) {
        *json_out = rhs;
        if (lhs->type() == cJSON_Object) { //CASE 3: a normal MUTATE
            if (!rhs_pk) { //CHECK 2: Primary key isn't removed
                throw runtime_exc_t(strprintf("mutate cannot remove the primary key (%s) of %s (new object: %s)",
                                              primary_key.c_str(), lhs->Print().c_str(), rhs->Print().c_str()), backtrace);
            }
            guarantee(cJSON_Equal(lhs_pk, rhs_pk)); //See CHECK 1 above
            return point_modify::MODIFIED;
        }

        //CASE 4: a deleting MUTATE
        guarantee(lhs->type() == cJSON_NULL);
        if (!rhs_pk) { //CHECK 3: Primary key exists for insert (no generation allowed);
            throw runtime_exc_t(strprintf("mutate must provide a primary key (%s), but there was none in: %s",
                                          primary_key.c_str(), rhs->Print().c_str()), backtrace);
        }
        *new_key_out = cJSON_print_primary(rhs_pk, backtrace); //CHECK 4: valid primary key for insert (may throw)
        return point_modify::INSERTED;
    } else { //CASE 5: a normal UPDATE
        guarantee(op == point_modify::UPDATE && lhs->type() == cJSON_Object);
        json_out->reset(new scoped_cJSON_t(cJSON_merge(lhs->get(), rhs->get())));
        guarantee_debug_throw_release(cJSON_Equal((*json_out)->GetObjectItem(primary_key.c_str()),
                                                  lhs->GetObjectItem(primary_key.c_str())), backtrace);
        return point_modify::MODIFIED;
    }
    //[point_modify::ERROR] never returned; we throw instead and the caller should catch it.
}

point_modify::result_t point_modify(namespace_repo_t<rdb_protocol_t>::access_t ns_access,
                                    const std::string &pk, cJSON *id, point_modify::op_t op,
                                    runtime_environment_t *env, const Mapping &m, const scopes_t &scopes,
                                    bool atomic, boost::shared_ptr<scoped_cJSON_t> row,
                                    const backtrace_t &backtrace) {
    guarantee_debug_throw_release(m.body().HasExtension(extension::deterministic), backtrace);
    bool deterministic = m.body().GetExtension(extension::deterministic);
    point_modify::result_t res;
    std::string opname = point_modify::op_name(op);
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
            case point_modify::INSERTED:
                if (!cJSON_Equal(new_row->GetObjectItem(pk.c_str()), id)) {
                    throw runtime_exc_t(strprintf("%s can't change the primary key (%s) (original: %s, new row: %s)",
                                                  opname.c_str(), pk.c_str(), cJSON_print_std_string(id).c_str(),
                                                  new_row->Print().c_str()), backtrace);
                }
                //FALLTHROUGH
            case point_modify::MODIFIED: {
                guarantee_debug_throw_release(new_row, backtrace);
                boost::optional<std::string> generated_key;
                throwing_insert(ns_access, pk, new_row, env, backtrace, true /*overwrite*/, &generated_key);
                guarantee(!generated_key); //overwriting inserts never generate keys
            } break;
            case point_modify::DELETED: {
                int num_deleted = point_delete(ns_access, id, env, backtrace);
                if (num_deleted == 0) res = point_modify::NOP;
            } break;
            case point_modify::SKIPPED: break;
            case point_modify::NOP: break;
            case point_modify::ERROR: unreachable("compute_modify should never return ERROR, it should throw");
            default: unreachable();
            }
            return res;
        } else { //deterministic modifications can be dispatched as point_modifies
            rdb_protocol_t::write_t write(rdb_protocol_t::point_modify_t(pk, store_key_t(cJSON_print_primary(id, backtrace)), op, scopes, backtrace, m));
            rdb_protocol_t::write_response_t response;
            ns_access.get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);
            rdb_protocol_t::point_modify_response_t mod_res = boost::get<rdb_protocol_t::point_modify_response_t>(response.response);
            if (mod_res.result == point_modify::ERROR) throw mod_res.exc;
            return mod_res.result;
        }
    } catch (const cannot_perform_query_exc_t &e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

int point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> id, runtime_environment_t *env, const backtrace_t &backtrace) {
    return point_delete(ns_access, id->get(), env, backtrace);
}

void execute_write_query(WriteQuery *w, runtime_environment_t *env, Response *res, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    res->set_status_code(Response::SUCCESS_JSON);
    switch (w->type()) {
    case WriteQuery::UPDATE: {
        view_t view = eval_term_as_view(w->mutable_update()->mutable_view(), env, scopes, backtrace.with("view"));
        std::string reported_error = "";

        int updated = 0, errors = 0, skipped = 0;
        while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
            guarantee_debug_throw_release(json && json->type() == cJSON_Object, backtrace);
            try {
                std::string pk = view.primary_key;
                cJSON *id = json->GetObjectItem(pk.c_str());
                point_modify::result_t mres =
                    point_modify(view.access, pk, id, point_modify::UPDATE, env, w->update().mapping(), scopes,
                                 w->atomic(), json,backtrace.with("modify_map"));
                guarantee(mres == point_modify::MODIFIED || mres == point_modify::SKIPPED);
                updated += (mres == point_modify::MODIFIED);
                skipped += (mres == point_modify::SKIPPED);
            } catch (const query_language::broken_client_exc_t &e) {
                ++errors;
                if (reported_error == "") reported_error = e.message;
            } catch (const query_language::runtime_exc_t &e) {
                ++errors;
                if (reported_error == "") reported_error = e.message + "\nBacktrace:\n" + e.backtrace.print();
            }
        }
        std::string res_list = strprintf("\"updated\": %d, \"skipped\": %d, \"errors\": %d", updated, skipped, errors);
        if (reported_error != "") {
            res_list = strprintf("%s, \"first_error\": %s", res_list.c_str(),
                                 scoped_cJSON_t(cJSON_CreateString(reported_error.c_str())).Print().c_str());
        }
        res->add_response("{" + res_list + "}");
    } break;
    case WriteQuery::MUTATE: {
        view_t view = eval_term_as_view(w->mutable_mutate()->mutable_view(), env, scopes, backtrace.with("view"));

        int modified = 0, deleted = 0, errors = 0;
        std::string reported_error = "";
        while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
            guarantee_debug_throw_release(json && json->type() == cJSON_Object, backtrace);
            try {
                std::string pk = view.primary_key;
                cJSON *id = json->GetObjectItem(pk.c_str());
                point_modify::result_t mres =
                    point_modify(view.access, pk, id, point_modify::MUTATE, env, w->mutate().mapping(), scopes,
                                 w->atomic(), json, backtrace.with("modify_map"));
                guarantee(mres == point_modify::MODIFIED || mres == point_modify::DELETED);
                modified += (mres == point_modify::MODIFIED);
                deleted += (mres == point_modify::DELETED);
            } catch (const query_language::broken_client_exc_t &e) {
                ++errors;
                if (reported_error == "") reported_error = e.message;
            } catch (const query_language::runtime_exc_t &e) {
                ++errors;
                if (reported_error == "") reported_error = e.message + "\nBacktrace:\n" + e.backtrace.print();
            }
        }
        std::string res_list = strprintf("\"modified\": %d, \"inserted\": %d, \"deleted\": %d, \"errors\": %d",
                                         modified, 0, deleted, errors);
        if (reported_error != "") {
            res_list = strprintf("%s, \"first_error\": %s", res_list.c_str(),
                                 scoped_cJSON_t(cJSON_CreateString(reported_error.c_str())).Print().c_str());
        }
        res->add_response("{" + res_list + "}");
    } break;
    case WriteQuery::DELETE: {
        view_t view = eval_term_as_view(w->mutable_delete_()->mutable_view(), env, scopes, backtrace.with("view"));

        int deleted = 0;
        while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
            deleted += point_delete(view.access, json->GetObjectItem(view.primary_key.c_str()), env, backtrace);
        }

        res->add_response(strprintf("{\"deleted\": %d}", deleted));
    } break;
    case WriteQuery::INSERT: {
        std::string pk = get_primary_key(w->mutable_insert()->mutable_table_ref(), env, backtrace);
        bool overwrite = w->mutable_insert()->overwrite();
        namespace_repo_t<rdb_protocol_t>::access_t ns_access =
            eval_table_ref(w->mutable_insert()->mutable_table_ref(), env, backtrace);

        std::string first_error;
        int errors = 0; //TODO: error reporting
        int inserted = 0;
        std::vector<std::string> generated_keys;
        if (w->insert().terms_size() == 1) {
            Term *t = w->mutable_insert()->mutable_terms(0);
            int32_t t_type = t->GetExtension(extension::inferred_type);
            boost::shared_ptr<json_stream_t> stream;
            if (t_type == TERM_TYPE_JSON) {
                boost::shared_ptr<scoped_cJSON_t> data = eval_term_as_json(t, env, scopes, backtrace.with("term:0"));
                if (data->type() == cJSON_Array) {
                    stream.reset(new in_memory_stream_t(json_array_iterator_t(data->get())));
                } else {
                    nonthrowing_insert(ns_access, pk, data, env, backtrace.with("term:0"), overwrite, &generated_keys,
                                       &inserted, &errors, &first_error);
                }
            } else if (t_type == TERM_TYPE_STREAM || t_type == TERM_TYPE_VIEW) {
                stream = eval_term_as_stream(w->mutable_insert()->mutable_terms(0), env, scopes, backtrace.with("term:0"));
            } else { unreachable("bad term type"); }
            if (stream) {
                while (boost::shared_ptr<scoped_cJSON_t> data = stream->next()) {
                    nonthrowing_insert(ns_access, pk, data, env, backtrace.with("term:0"), overwrite, &generated_keys,
                                       &inserted, &errors, &first_error);
                }
            }
        } else {
            for (int i = 0; i < w->insert().terms_size(); ++i) {
                boost::shared_ptr<scoped_cJSON_t> data =
                    eval_term_as_json(w->mutable_insert()->mutable_terms(i), env, scopes, backtrace.with(strprintf("term:%d", i)));
                nonthrowing_insert(ns_access, pk, data, env, backtrace.with(strprintf("term:%d", i)), overwrite, &generated_keys,
                                   &inserted, &errors, &first_error);
            }
        }

        /* Construct a response. */
        boost::shared_ptr<scoped_cJSON_t> res_json(new scoped_cJSON_t(cJSON_CreateObject()));
        res_json->AddItemToObject("inserted", safe_cJSON_CreateNumber(inserted, backtrace));
        res_json->AddItemToObject("errors", safe_cJSON_CreateNumber(errors, backtrace));

        if (first_error != "") res_json->AddItemToObject("first_error", cJSON_CreateString(first_error.c_str()));
        if (!generated_keys.empty()) {
            res_json->AddItemToObject("generated_keys", cJSON_CreateArray());
            cJSON *array = res_json->GetObjectItem("generated_keys");
            guarantee(array);
            for (std::vector<std::string>::iterator it = generated_keys.begin(); it != generated_keys.end(); ++it) {
                cJSON_AddItemToArray(array, cJSON_CreateString(it->c_str()));
            }
        }
        res->add_response(res_json->Print());
    } break;
    case WriteQuery::FOREACH: {
        boost::shared_ptr<json_stream_t> stream =
            eval_term_as_stream(w->mutable_for_each()->mutable_stream(), env, scopes, backtrace.with("stream"));

        // The cJSON object we'll eventually return (named `lhs` because it's merged with `rhs` below).
        scoped_cJSON_t lhs(cJSON_CreateObject());

        scopes_t scopes_copy = scopes;
        while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            variable_type_scope_t::new_scope_t type_scope_maker(&scopes_copy.type_env.scope, w->for_each().var(), term_info_t(TERM_TYPE_JSON, true));
            variable_val_scope_t::new_scope_t scope_maker(&scopes_copy.scope, w->for_each().var(), json);

            for (int i = 0; i < w->for_each().queries_size(); ++i) {
                // Run the write query and retrieve the result.
                guarantee_debug_throw_release(res->response_size() == 0, backtrace);
                scoped_cJSON_t rhs(0);
                try {
                    execute_write_query(w->mutable_for_each()->mutable_queries(i), env, res, scopes_copy, backtrace.with(strprintf("query:%d", i)));
                    guarantee_debug_throw_release(res->response_size() == 1, backtrace);
                    rhs.reset(cJSON_Parse(res->response(0).c_str()));
                } catch (const runtime_exc_t &e) {
                    rhs.reset(cJSON_CreateObject());
                    rhs.AddItemToObject("errors", safe_cJSON_CreateNumber(1.0, backtrace));
                    std::string err = strprintf("Term %d of the foreach threw an error: %s\n", i,
                                                (e.message + "\nBacktrace:\n" + e.backtrace.print()).c_str());
                    rhs.AddItemToObject("first_error", cJSON_CreateString(err.c_str()));
                }
                res->clear_response();

                // Merge the result of the write query into `lhs`
                scoped_cJSON_t merged(cJSON_merge(lhs.get(), rhs.get()));
                for (int f = 0; f < merged.GetArraySize(); ++f) {
                    cJSON *el = merged.GetArrayItem(f);
                    guarantee_debug_throw_release(el, backtrace);
                    cJSON *lhs_el = lhs.GetObjectItem(el->string);
                    cJSON *rhs_el = rhs.GetObjectItem(el->string);
                    if (lhs_el && lhs_el->type == cJSON_Number &&
                        rhs_el && rhs_el->type == cJSON_Number) {
                        el->valueint = lhs_el->valueint + rhs_el->valueint;
                        el->valuedouble = lhs_el->valuedouble + rhs_el->valuedouble;
                    }
                }
                lhs.reset(merged.release());
            }
        }
        res->add_response(lhs.Print());
    } break;
    case WriteQuery::POINTUPDATE: {
        namespace_repo_t<rdb_protocol_t>::access_t ns_access =
            eval_table_ref(w->mutable_point_update()->mutable_table_ref(), env, backtrace);
        std::string pk = get_primary_key(w->mutable_point_update()->mutable_table_ref(), env, backtrace);
        std::string attr = w->mutable_point_update()->attrname();
        if (attr != "" && attr != pk) {
            throw runtime_exc_t(strprintf("Attribute %s is not a primary key (options: %s).", attr.c_str(), pk.c_str()),
                                backtrace.with("keyname"));
        }
        boost::shared_ptr<scoped_cJSON_t> id = eval_term_as_json(w->mutable_point_update()->mutable_key(),
                                                                 env, scopes, backtrace.with("key"));
        point_modify::result_t mres =
            point_modify(ns_access, pk, id->get(), point_modify::UPDATE, env, w->point_update().mapping(), scopes,
                         w->atomic(), boost::shared_ptr<scoped_cJSON_t>(), backtrace.with("point_map"));
        guarantee(mres == point_modify::MODIFIED || mres == point_modify::SKIPPED);
        res->add_response(strprintf("{\"updated\": %d, \"skipped\": %d, \"errors\": %d}",
                                    mres == point_modify::MODIFIED, mres == point_modify::SKIPPED, 0));
    } break;
    case WriteQuery::POINTDELETE: { //TODO: enforce primary key
        int deleted = -1;
        std::string pk = get_primary_key(w->mutable_point_delete()->mutable_table_ref(), env, backtrace);
        std::string attr = w->mutable_point_delete()->attrname();
        if (attr != "" && attr != pk) {
            throw runtime_exc_t(strprintf("Attribute %s is not a primary key (options: %s).", attr.c_str(), pk.c_str()),
                                backtrace.with("keyname"));
        }
        namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval_table_ref(w->mutable_point_delete()->mutable_table_ref(), env, backtrace);
        boost::shared_ptr<scoped_cJSON_t> id = eval_term_as_json(w->mutable_point_delete()->mutable_key(), env, scopes, backtrace.with("key"));
        deleted = point_delete(ns_access, id, env, backtrace);
        guarantee_debug_throw_release(deleted == 0 || deleted == 1, backtrace);

        res->add_response(strprintf("{\"deleted\": %d}", deleted));
    } break;
    case WriteQuery::POINTMUTATE: {
        namespace_repo_t<rdb_protocol_t>::access_t ns_access =
            eval_table_ref(w->mutable_point_mutate()->mutable_table_ref(), env, backtrace);
        std::string pk = get_primary_key(w->mutable_point_mutate()->mutable_table_ref(), env, backtrace);
        std::string attr = w->mutable_point_mutate()->attrname();
        if (attr != "" && attr != pk) {
            throw runtime_exc_t(strprintf("Attribute %s is not a primary key (options: %s).", attr.c_str(), pk.c_str()),
                                backtrace.with("keyname"));
        }
        boost::shared_ptr<scoped_cJSON_t> id = eval_term_as_json(w->mutable_point_mutate()->mutable_key(),
                                                                 env, scopes, backtrace.with("key"));
        point_modify::result_t mres =
            point_modify(ns_access, pk, id->get(), point_modify::MUTATE, env, w->point_mutate().mapping(), scopes,
                         w->atomic(), boost::shared_ptr<scoped_cJSON_t>(), backtrace.with("point_map"));
        guarantee(mres == point_modify::MODIFIED || mres == point_modify::INSERTED ||
                           mres == point_modify::DELETED  || mres == point_modify::NOP);
        res->add_response(strprintf("{\"modified\": %d, \"inserted\": %d, \"deleted\": %d, \"errors\": %d}",
                                    mres == point_modify::MODIFIED, mres == point_modify::INSERTED, mres == point_modify::DELETED, 0));
    } break;
    default:
        unreachable();
    }
}

//This doesn't create a scope for the evals
void eval_let_binds(Term::Let *let, runtime_environment_t *env, scopes_t *scopes_in_out, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    // Go through the bindings in a let and add them one by one
    for (int i = 0; i < let->binds_size(); ++i) {
        backtrace_t backtrace_bind = backtrace.with(strprintf("bind:%s", let->binds(i).var().c_str()));
        term_info_t type = get_term_type(let->mutable_binds(i)->mutable_term(), &scopes_in_out->type_env, backtrace_bind);

        if (type.type == TERM_TYPE_JSON) {
            scopes_in_out->scope.put_in_scope(let->binds(i).var(),
                    eval_term_as_json(let->mutable_binds(i)->mutable_term(), env, *scopes_in_out, backtrace_bind));
        } else if (type.type == TERM_TYPE_STREAM || type.type == TERM_TYPE_VIEW) {
            throw runtime_exc_t("Cannot bind streams/views to variable names", backtrace);
        } else if (type.type == TERM_TYPE_ARBITRARY) {
            eval_term_as_json(let->mutable_binds(i)->mutable_term(), env, *scopes_in_out, backtrace_bind);
            unreachable("This term has type `TERM_TYPE_ARBITRARY`, so "
                "evaluating it must throw  `runtime_exc_t`.");
        }

        scopes_in_out->type_env.scope.put_in_scope(let->binds(i).var(), type);
    }
}

boost::shared_ptr<scoped_cJSON_t> eval_term_as_json_and_check(Term *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, int type, const std::string &msg) {
    boost::shared_ptr<scoped_cJSON_t> res = eval_term_as_json(t, env, scopes, backtrace);
    if (res->type() != type) {
        throw runtime_exc_t(msg, backtrace);
    }
    return res;
}

boost::shared_ptr<scoped_cJSON_t> eval_term_as_json_and_check_either(Term *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, int type1, int type2, const std::string &msg) {
    boost::shared_ptr<scoped_cJSON_t> res = eval_term_as_json(t, env, scopes, backtrace);
    if (res->type() != type1 && res->type() != type2) {
        throw runtime_exc_t(msg, backtrace);
    }
    return res;
}


boost::shared_ptr<scoped_cJSON_t> eval_term_as_json(Term *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    switch (t->type()) {
    case Term::IMPLICIT_VAR:
        guarantee_debug_throw_release(scopes.implicit_attribute_value.has_value(), backtrace);
        return scopes.implicit_attribute_value.get_value();
    case Term::VAR: {
        boost::shared_ptr<scoped_cJSON_t> ptr = scopes.scope.get(t->var());
        guarantee_debug_throw_release(ptr.get() && ptr->get(), backtrace);
        return ptr;
    } break;
    case Term::LET:
        {
            // Push the scope
            scopes_t scopes_copy = scopes;
            new_scope_t inner_type_scope(&scopes_copy.type_env.scope);
            new_val_scope_t inner_val_scope(&scopes_copy.scope);

            eval_let_binds(t->mutable_let(), env, &scopes_copy, backtrace);

            return eval_term_as_json(t->mutable_let()->mutable_expr(), env, scopes_copy, backtrace.with("expr"));
        }
        break;
    case Term::CALL:
        return eval_call_as_json(t->mutable_call(), env, scopes, backtrace);
        break;
    case Term::IF:
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
    case Term::ERROR:
        throw runtime_exc_t(t->error(), backtrace);
        break;
    case Term::NUMBER:
        {
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(safe_cJSON_CreateNumber(t->number(), backtrace)));
        }
        break;
    case Term::STRING:
        {
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateString(t->valuestring().c_str())));
        }
        break;
    case Term::JSON: {
        const char *str = t->jsonstring().c_str();
        boost::shared_ptr<scoped_cJSON_t> json(new scoped_cJSON_t(cJSON_Parse(str)));
        if (!json->get()) {
            throw runtime_exc_t(strprintf("Malformed JSON: %s", str), backtrace);
        }
        return json;
    } break;
    case Term::BOOL:
        {
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateBool(t->valuebool())));
        }
        break;
    case Term::JSON_NULL:
        {
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNull()));
        }
        break;
    case Term::ARRAY:
        {
            boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));
            for (int i = 0; i < t->array_size(); ++i) {
                res->AddItemToArray(eval_term_as_json(t->mutable_array(i), env, scopes, backtrace.with(strprintf("elem:%d", i)))->DeepCopy());
            }
            return res;
        }
        break;
    case Term::OBJECT:
        {
            boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateObject()));
            for (int i = 0; i < t->object_size(); ++i) {
                std::string item_name(t->object(i).var());
                res->AddItemToObject(item_name.c_str(), eval_term_as_json(t->mutable_object(i)->mutable_term(), env, scopes, backtrace.with(strprintf("key:%s", item_name.c_str())))->DeepCopy());
            }
            return res;
        }
        break;
    case Term::GETBYKEY:
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
    case Term::TABLE:
        crash("Term::TABLE must be evaluated with eval_stream or eval_view");

    case Term::JAVASCRIPT: {
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
                throw runtime_exc_t("failed to compile javascript: " + errmsg, backtrace);
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
        result = js->call(id, object, argvals, &errmsg);
        if (!result) {
            throw runtime_exc_t("failed to evaluate javascript: " + errmsg, backtrace);
        }
        return result;
    }

    default:
        unreachable();
    }
    unreachable();
}

boost::shared_ptr<json_stream_t> eval_term_as_stream(Term *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    switch (t->type()) {
    case Term::LET:
        {
            // Push the scope
            scopes_t scopes_copy = scopes;
            new_scope_t inner_type_scope(&scopes_copy.type_env.scope);
            new_val_scope_t inner_val_scope(&scopes_copy.scope);

            eval_let_binds(t->mutable_let(), env, &scopes_copy, backtrace);

            return eval_term_as_stream(t->mutable_let()->mutable_expr(), env, scopes_copy, backtrace.with("expr"));
        }
        break;
    case Term::CALL:
        return eval_call_as_stream(t->mutable_call(), env, scopes, backtrace);
        break;
    case Term::IF:
        {
            boost::shared_ptr<scoped_cJSON_t> test = eval_term_as_json_and_check_either(t->mutable_if_()->mutable_test(), env, scopes, backtrace.with("test"), cJSON_True, cJSON_False, "The IF test must evaluate to a boolean.");

            if (test->type() == cJSON_True) {
                return eval_term_as_stream(t->mutable_if_()->mutable_true_branch(), env, scopes, backtrace.with("true"));
            } else {
                return eval_term_as_stream(t->mutable_if_()->mutable_false_branch(), env, scopes, backtrace.with("false"));
            }
        }
        break;
    case Term::TABLE:
        {
            return eval_table_as_view(t->mutable_table(), env, backtrace).stream;
        }
        break;
    case Term::IMPLICIT_VAR:
    case Term::VAR:
    case Term::ERROR:
    case Term::NUMBER:
    case Term::STRING:
    case Term::JSON:
    case Term::BOOL:
    case Term::JSON_NULL:
    case Term::OBJECT:
    case Term::GETBYKEY:
    case Term::JAVASCRIPT:
    case Term::ARRAY: {
        /* TODO: The type checker should have rejected these cases. Why are
           we handling them? */
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

boost::shared_ptr<scoped_cJSON_t> eval_call_as_json(Term::Call *c, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    switch (c->builtin().type()) {
        //JSON -> JSON
        case Builtin::NOT:
            {
                boost::shared_ptr<scoped_cJSON_t> data = eval_term_as_json(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                if (data->get()->type == cJSON_False) {
                    data->get()->type = cJSON_True;
                } else if (data->get()->type == cJSON_True) {
                    data->get()->type = cJSON_False;
                } else {
                    throw runtime_exc_t("Not can only be called on a boolean", backtrace.with("arg:0"));
                }
                return data;
            }
            break;
        case Builtin::GETATTR:
        case Builtin::IMPLICIT_GETATTR:
            {
                boost::shared_ptr<scoped_cJSON_t> data;
                if (c->builtin().type() == Builtin::GETATTR) {
                    data = eval_term_as_json(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
                } else {
                    guarantee_debug_throw_release(scopes.implicit_attribute_value.has_value(), backtrace);
                    data = scopes.implicit_attribute_value.get_value();
                }

                if (data->type() != cJSON_Object) {
                    throw runtime_exc_t("Data: \n" + data->Print() + "\nmust be an object", backtrace.with("arg:0"));
                }

                cJSON *value = data->GetObjectItem(c->builtin().attr().c_str());

                if (!value) {
                    throw runtime_exc_t("Object:\n" + data->Print() +"\nis missing attribute \"" + c->builtin().attr() + "\"", backtrace.with("attr"));
                }

                return shared_scoped_json(cJSON_DeepCopy(value));
            }
            break;
        case Builtin::HASATTR:
        case Builtin::IMPLICIT_HASATTR:
            {
                boost::shared_ptr<scoped_cJSON_t> data;
                if (c->builtin().type() == Builtin::HASATTR) {
                    data = eval_term_as_json(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
                } else {
                    guarantee_debug_throw_release(scopes.implicit_attribute_value.has_value(), backtrace);
                    data = scopes.implicit_attribute_value.get_value();
                }

                if (data->type() != cJSON_Object) {
                    throw runtime_exc_t("Data: \n" + data->Print() + "\nmust be an object", backtrace.with("arg:0"));
                }

                cJSON *attr = data->GetObjectItem(c->builtin().attr().c_str());

                if (attr) {
                    return shared_scoped_json(cJSON_CreateTrue());
                } else {
                    return shared_scoped_json(cJSON_CreateFalse());
                }
            }
            break;
        case Builtin::IMPLICIT_WITHOUT:
        case Builtin::WITHOUT: {
            boost::shared_ptr<scoped_cJSON_t> data;
            if (c->builtin().type() == Builtin::WITHOUT) {
                data = eval_term_as_json(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
            } else {
                guarantee(c->builtin().type() == Builtin::IMPLICIT_WITHOUT);
                guarantee_debug_throw_release(scopes.implicit_attribute_value.has_value(), backtrace);
                data = scopes.implicit_attribute_value.get_value();
            }
            if (data->type() != cJSON_Object) {
                throw runtime_exc_t("Data: \n" + data->Print() + "\nmust be an object", backtrace.with("arg:0"));
            }
            boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(data->DeepCopy()));
            for (int i = 0; i < c->builtin().attrs_size(); ++i) {
                res->DeleteItemFromObject(c->builtin().attrs(i).c_str());
            }
            return res;
        } break;
        case Builtin::PICKATTRS:
        case Builtin::IMPLICIT_PICKATTRS:
            {
                boost::shared_ptr<scoped_cJSON_t> data;
                if (c->builtin().type() == Builtin::PICKATTRS) {
                    data = eval_term_as_json(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
                } else {
                    guarantee_debug_throw_release(scopes.implicit_attribute_value.has_value(), backtrace);
                    data = scopes.implicit_attribute_value.get_value();
                }

                if (data->type() != cJSON_Object) {
                    throw runtime_exc_t("Data: \n" + data->Print() + "\nmust be an object", backtrace.with("arg:0"));
                }

                boost::shared_ptr<scoped_cJSON_t> res = shared_scoped_json(cJSON_CreateObject());

                for (int i = 0; i < c->builtin().attrs_size(); ++i) {
                    cJSON *item = data->GetObjectItem(c->builtin().attrs(i).c_str());
                    if (!item) {
                        throw runtime_exc_t("Attempting to pick missing attribute " + c->builtin().attrs(i) + " from data:\n" + data->Print(), backtrace.with(strprintf("attrs:%d", i)));
                    } else {
                        res->AddItemToObject(item->string, cJSON_DeepCopy(item));
                    }
                }
                return res;
            }
            break;
        case Builtin::MAPMERGE:
            {
                boost::shared_ptr<scoped_cJSON_t> left  = eval_term_as_json_and_check(c->mutable_args(0), env, scopes, backtrace.with("arg:0"),
                                                                                      cJSON_Object, "Data must be an object"),
                                                  right = eval_term_as_json_and_check(c->mutable_args(1), env, scopes, backtrace.with("arg:1"),
                                                                                      cJSON_Object, "Data must be an object");
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_merge(left->get(), right->get())));
            }
            break;
        case Builtin::ARRAYAPPEND:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array = eval_term_as_json_and_check(c->mutable_args(0), env, scopes, backtrace.with("arg:0"),
                    cJSON_Array, "The first argument must be an array.");
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(array->DeepCopy()));
                res->AddItemToArray(eval_term_as_json(c->mutable_args(1), env, scopes, backtrace.with("arg:1"))->DeepCopy());
                return res;
            }
            break;
        case Builtin::SLICE:
            {
                int start, stop;

                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array = eval_term_as_json_and_check(c->mutable_args(0), env, scopes, backtrace.with("arg:0"),
                                                                                      cJSON_Array, "The first argument must be an array.");

                int length = array->GetArraySize();

                // Check second arg type
                {
                    boost::shared_ptr<scoped_cJSON_t> start_json = eval_term_as_json_and_check_either(c->mutable_args(1), env, scopes, backtrace.with("arg:1"), cJSON_NULL, cJSON_Number, "Slice start must be null or an integer.");
                    if (start_json->type() == cJSON_NULL) {
                        start = 0;
                    } else {    // cJSON_Number
                        float float_start = start_json->get()->valuedouble;
                        start = static_cast<int>(float_start);
                        if (float_start != start) {
                            throw runtime_exc_t("Slice start must be null or an integer.", backtrace.with("arg:1"));
                        }
                    }
                }

                // Check third arg type
                {
                    boost::shared_ptr<scoped_cJSON_t> stop_json = eval_term_as_json_and_check_either(c->mutable_args(2), env, scopes, backtrace.with("arg:2"), cJSON_NULL, cJSON_Number, "Slice stop must be null or an integer.");
                    if (stop_json->type() == cJSON_NULL) {
                        stop = length;
                    } else {
                        float float_stop = stop_json->get()->valuedouble;
                        stop = static_cast<int>(float_stop);
                        if (float_stop != stop) {
                            throw runtime_exc_t("Slice stop must be null or an integer.", backtrace.with("arg:2"));
                        }
                    }
                }

                if (start < 0) {
                    start = std::max(start + length, 0);
                }
                if (start > length) {
                    start = length;
                }

                if (stop < 0) {
                    stop = std::max(stop + length, 0);
                }
                if (stop > length) {
                    stop = length;
                }

                if (start == 0 && stop == length) {
                    return array;   // nothing to do
                }

                // Create a new array and slice the elements into it
                if (start > stop) {
                    throw runtime_exc_t("The second argument cannot be greater than the third argument.", backtrace.with("arg:2"));
                }
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));
                for(int i = start; i < stop; i++) {
                    res->AddItemToArray(cJSON_DeepCopy(array->GetArrayItem(i)));
                }

                return res;
            }
            break;
        case Builtin::ADD:
            {
                if (c->args_size() == 0) {
                    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNull()));
                }

                boost::shared_ptr<scoped_cJSON_t> arg = eval_term_as_json(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                if (arg->type() == cJSON_Number) {
                    double result = arg->get()->valuedouble;

                    for (int i = 1; i < c->args_size(); ++i) {
                        arg = eval_term_as_json_and_check(c->mutable_args(i), env, scopes, backtrace.with(strprintf("arg:%d", i)), cJSON_Number, "Cannot ADD numbers to non-numbers");
                        result += arg->get()->valuedouble;
                    }

                    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(safe_cJSON_CreateNumber(result, backtrace)));
                } else if (arg->type() == cJSON_Array) {
                    boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(arg->DeepCopy()));
                    for (int i = 1; i < c->args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg2 = eval_term_as_json_and_check(c->mutable_args(i), env, scopes, backtrace.with(strprintf("arg:%d", i)), cJSON_Array, "Cannot ADD arrays to non-arrays");
                        for(int j = 0; j < arg2->GetArraySize(); ++j) {
                            res->AddItemToArray(cJSON_DeepCopy(arg2->GetArrayItem(j)));
                        }
                    }
                    return res;
                } else {
                    throw runtime_exc_t("Can only ADD numbers with numbers and arrays with arrays", backtrace.with("arg:0"));
                }
            }
            break;
        case Builtin::SUBTRACT:
            {
                double result = 0.0;

                if (c->args_size() > 0) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval_term_as_json_and_check(c->mutable_args(0), env, scopes, backtrace.with("arg:0"), cJSON_Number, "All operands to SUBTRACT must be numbers.");
                    if (c->args_size() == 1) {
                        result = -arg->get()->valuedouble;  // (- x) is negate
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c->args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg2 = eval_term_as_json_and_check(c->mutable_args(i), env, scopes, backtrace.with(strprintf("arg:%d", i)), cJSON_Number, "All operands to SUBTRACT must be numbers.");
                        result -= arg2->get()->valuedouble;
                    }
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(safe_cJSON_CreateNumber(result, backtrace)));
                return res;
            }
            break;
        case Builtin::MULTIPLY:
            {
                double result = 1.0;

                for (int i = 0; i < c->args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval_term_as_json_and_check(c->mutable_args(i), env, scopes, backtrace.with(strprintf("arg:%d", i)), cJSON_Number, "All operands of MULTIPLY must be numbers.");
                    result *= arg->get()->valuedouble;
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(safe_cJSON_CreateNumber(result, backtrace)));
                return res;
            }
            break;
        case Builtin::DIVIDE:
            {
                double result = 0.0;

                if (c->args_size() > 0) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval_term_as_json_and_check(c->mutable_args(0), env, scopes, backtrace.with("arg:0"), cJSON_Number, "All operands to DIVIDE must be numbers.");
                    if (c->args_size() == 1) {
                        result = 1.0 / arg->get()->valuedouble;  // (/ x) is reciprocal
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c->args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg2 = eval_term_as_json_and_check(c->mutable_args(i), env, scopes, backtrace.with(strprintf("arg:%d", i)), cJSON_Number, "All operands to DIVIDE must be numbers.");
                        result /= arg2->get()->valuedouble;
                    }
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(safe_cJSON_CreateNumber(result, backtrace)));
                return res;
            }
            break;
        case Builtin::MODULO:
            {
                boost::shared_ptr<scoped_cJSON_t> lhs = eval_term_as_json_and_check(c->mutable_args(0), env, scopes, backtrace.with("arg:0"), cJSON_Number, "First operand of MOD must be a number."),
                                                  rhs = eval_term_as_json_and_check(c->mutable_args(1), env, scopes, backtrace.with("arg:1"), cJSON_Number, "Second operand of MOD must be a number.");

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(safe_cJSON_CreateNumber(fmod(lhs->get()->valuedouble, rhs->get()->valuedouble), backtrace)));
                return res;
            }
            break;
        case Builtin::COMPARE:
            {
                bool result = true;

                boost::shared_ptr<scoped_cJSON_t> lhs = eval_term_as_json(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                for (int i = 1; i < c->args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> rhs = eval_term_as_json(c->mutable_args(i), env, scopes, backtrace.with(strprintf("arg:%d", i)));

                    int res = lhs->type() == cJSON_NULL && rhs->type() == cJSON_NULL ? 0:
                        cJSON_cmp(lhs->get(), rhs->get(), backtrace.with(strprintf("arg:%d", i)));

                    switch (c->builtin().comparison()) {
                    case Builtin_Comparison_EQ:
                        result &= (res == 0);
                        break;
                    case Builtin_Comparison_NE:
                        result &= (res != 0);
                        break;
                    case Builtin_Comparison_LT:
                        result &= (res < 0);
                        break;
                    case Builtin_Comparison_LE:
                        result &= (res <= 0);
                        break;
                    case Builtin_Comparison_GT:
                        result &= (res > 0);
                        break;
                    case Builtin_Comparison_GE:
                        result &= (res >= 0);
                        break;
                    default:
                        crash("Unknown comparison operator.");
                        break;
                    }

                    if (!result)
                        break;

                    lhs = rhs;
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateBool(result)));
            }
            break;
        case Builtin::FILTER:
        case Builtin::MAP:
        case Builtin::CONCATMAP:
        case Builtin::ORDERBY:
        case Builtin::DISTINCT:
        case Builtin::UNION:
        case Builtin::RANGE: {
            boost::shared_ptr<json_stream_t> stream = eval_call_as_stream(c, env, scopes, backtrace);
            boost::shared_ptr<scoped_cJSON_t>
                arr(new scoped_cJSON_t(cJSON_CreateArray()));
            boost::shared_ptr<scoped_cJSON_t> json;
            while ((json = stream->next())) {
                arr->AddItemToArray(json->DeepCopy());
            }
            return arr;
        } break;
        case Builtin::ARRAYTOSTREAM:
            unreachable("eval_term_as_json called on a function that returns a stream (use eval_term_as_stream instead).");
            break;
        case Builtin::LENGTH:
            {
                int length = 0;

                if (c->args(0).GetExtension(extension::inferred_type) == TERM_TYPE_JSON)
                {
                    // Check first arg type
                    boost::shared_ptr<scoped_cJSON_t> array = eval_term_as_json_and_check(c->mutable_args(0), env, scopes, backtrace.with("arg:0"), cJSON_Array, "LENGTH argument must be an array.");
                    length = array->GetArraySize();
                } else {
                    boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
                    while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                        ++length;
                    }
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(safe_cJSON_CreateNumber(length, backtrace)));
            }
            break;
        case Builtin::NTH:
            if (c->args(0).GetExtension(extension::inferred_type) == TERM_TYPE_JSON) {
                boost::shared_ptr<scoped_cJSON_t> array = eval_term_as_json_and_check(c->mutable_args(0), env, scopes, backtrace.with("arg:0"),
                    cJSON_Array, "The first argument must be an array.");

                boost::shared_ptr<scoped_cJSON_t> index_json = eval_term_as_json_and_check(c->mutable_args(1), env, scopes, backtrace.with("arg:1"),
                    cJSON_Number, "The second argument must be an integer.");

                float float_index = index_json->get()->valuedouble;
                int index = static_cast<int>(float_index);
                if (float_index != index) {
                    throw runtime_exc_t("The second argument must be an integer.", backtrace.with("arg:1"));
                }

                int length = array->GetArraySize();

                if (index < 0) {
                    index += length;
                }
                if (index < 0 || index >= length) {
                    throw runtime_exc_t("Array index out of bounds.", backtrace.with("arg:1"));
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_DeepCopy(array->GetArrayItem(index))));
            } else {
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> index_json = eval_term_as_json_and_check(c->mutable_args(1), env, scopes, backtrace.with("arg:1"), cJSON_Number, "The second argument must be an integer.");
                float index_float = index_json->get()->valuedouble;
                int index = static_cast<int>(index_float);
                if (index_float != index || index < 0) {
                    throw runtime_exc_t("The second argument must be a nonnegative integer.", backtrace.with("arg:1"));
                }

                boost::shared_ptr<scoped_cJSON_t> json;
                for (int i = 0; i <= index; ++i) {
                    json = stream->next();
                    if (!json) {
                        throw runtime_exc_t("Index out of bounds.", backtrace.with("arg:1"));
                    }
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(json->DeepCopy()));
            }
            break;
        case Builtin::STREAMTOARRAY:
            {
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    res->AddItemToArray(json->DeepCopy());
                }

                return res;
            }
            break;
        case Builtin::REDUCE:
            {
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                try {
                    return boost::get<boost::shared_ptr<scoped_cJSON_t> >(stream->apply_terminal(c->builtin().reduce(), env, scopes, backtrace.with("reduce")));
                } catch (const boost::bad_get &) {
                    crash("Expected a json atom... something is implemented wrong in the clustering code\n");
                }
            }
            break;
        case Builtin::GROUPEDMAPREDUCE: {
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                try {
                    rdb_protocol_t::rget_read_response_t::result_t result = stream->apply_terminal(c->builtin().grouped_map_reduce(), env, scopes, backtrace);
                    rdb_protocol_t::rget_read_response_t::groups_t *groups = boost::get<rdb_protocol_t::rget_read_response_t::groups_t>(&result);
                    boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));
                    std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less_t>::iterator it;
                    for (it = groups->begin(); it != groups->end(); ++it) {
                        scoped_cJSON_t obj(cJSON_CreateObject());
                        obj.AddItemToObject("group", it->first->release());
                        obj.AddItemToObject("reduction", it->second->release());
                        res->AddItemToArray(obj.release());
                    }
                    return res;
                } catch (const boost::bad_get &) {
                    crash("Expected a json atom... something is implemented wrong in the clustering code\n");
                }
        } break;
        case Builtin::ALL:
            {
                bool result = true;

                for (int i = 0; i < c->args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval_term_as_json(c->mutable_args(i), env, scopes, backtrace.with(strprintf("arg:%d", i)));
                    if (arg->type() != cJSON_False && arg->type() != cJSON_True) {
                        throw runtime_exc_t("All operands to ALL must be booleans.", backtrace.with(strprintf("arg:%d", i)));
                    }
                    if (arg->type() != cJSON_True) {
                        result = false;
                        break;
                    }
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateBool(result)));
                return res;
            }
            break;
        case Builtin::ANY:
            {
                bool result = false;

                for (int i = 0; i < c->args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval_term_as_json(c->mutable_args(i), env, scopes, backtrace.with(strprintf("arg:%d", i)));
                    if (arg->type() != cJSON_False && arg->type() != cJSON_True) {
                        throw runtime_exc_t("All operands to ANY must be booleans.", backtrace.with(strprintf("arg:%d", i)));
                    }
                    if (arg->type() == cJSON_True) {
                        result = true;
                        break;
                    }
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateBool(result)));
                return res;
            }
            break;
        default:
            crash("unreachable");
            break;
    }
    crash("unreachable");
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

class ordering_t {
public:
    ordering_t(const google::protobuf::RepeatedPtrField<Builtin::OrderBy> &_order, const backtrace_t &bt)
        : order(_order), backtrace(bt)
    { }

    //returns true if x < y according to the ordering
    bool operator()(const boost::shared_ptr<scoped_cJSON_t> &x, const boost::shared_ptr<scoped_cJSON_t> &y) const {
        if (x->type() != cJSON_Object) {
            throw runtime_exc_t(
                strprintf("Orderby encountered a non-object %s.\n", x->Print().c_str()),
                backtrace);
        } else if (y->type() != cJSON_Object) {
            throw runtime_exc_t(
                strprintf("Orderby encountered a non-object %s.\n", y->Print().c_str()),
                backtrace);
        }
        for (int i = 0; i < order.size(); ++i) {
            const Builtin::OrderBy& cur = order.Get(i);

            cJSON *a = cJSON_GetObjectItem(x->get(), cur.attr().c_str());
            cJSON *b = cJSON_GetObjectItem(y->get(), cur.attr().c_str());

            if (a == NULL || b == NULL) {
                std::string str = strprintf("ORDERBY encountered a row missing attr '%s': %s\n", cur.attr().c_str(),
                                            (a == NULL ? x->Print().c_str() : y->Print().c_str()));
                throw runtime_exc_t(str, backtrace);
            }

            int cmp = cJSON_cmp(a, b, backtrace);
            if (cmp) {
                return (cmp > 0) ^ cur.ascending();
            }
        }

        return false;
    }

private:
    const google::protobuf::RepeatedPtrField<Builtin::OrderBy> &order;
    backtrace_t backtrace;
};

boost::shared_ptr<scoped_cJSON_t> map(std::string arg, Term *term, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, boost::shared_ptr<scoped_cJSON_t> val) {
    scopes_t scopes_copy = scopes;

    variable_val_scope_t::new_scope_t scope_maker(&scopes_copy.scope, arg, val);
    variable_type_scope_t::new_scope_t type_scope_maker(&scopes_copy.type_env.scope, arg, term_info_t(TERM_TYPE_JSON, true));

    implicit_value_setter_t impliciter(&scopes_copy.implicit_attribute_value, val);
    implicit_type_t::impliciter_t type_impliciter(&scopes_copy.type_env.implicit_type, term_info_t(TERM_TYPE_JSON, true));
    return eval_term_as_json(term, env, scopes_copy, backtrace);
}

boost::shared_ptr<scoped_cJSON_t> eval_mapping(Mapping m, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace,
                                               boost::shared_ptr<scoped_cJSON_t> val) {
    return map(m.arg(), m.mutable_body(), env, scopes, backtrace, val);
}

boost::shared_ptr<json_stream_t> concatmap(std::string arg, Term *term, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace, boost::shared_ptr<scoped_cJSON_t> val) {
    scopes_t scopes_copy = scopes;
    variable_val_scope_t::new_scope_t scope_maker(&scopes_copy.scope, arg, val);
    variable_type_scope_t::new_scope_t type_scope_maker(&scopes_copy.type_env.scope, arg, term_info_t(TERM_TYPE_JSON, true));

    implicit_value_setter_t impliciter(&scopes_copy.implicit_attribute_value, val);
    implicit_type_t::impliciter_t type_impliciter(&scopes_copy.type_env.implicit_type, term_info_t(TERM_TYPE_JSON, true));
    return eval_term_as_stream(term, env, scopes_copy, backtrace);
}

boost::shared_ptr<json_stream_t> eval_call_as_stream(Term::Call *c, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    switch (c->builtin().type()) {
        //JSON -> JSON
        case Builtin::NOT:
        case Builtin::GETATTR:
        case Builtin::IMPLICIT_GETATTR:
        case Builtin::HASATTR:
        case Builtin::IMPLICIT_HASATTR:
        case Builtin::WITHOUT:
        case Builtin::IMPLICIT_WITHOUT:
        case Builtin::PICKATTRS:
        case Builtin::IMPLICIT_PICKATTRS:
        case Builtin::MAPMERGE:
        case Builtin::ARRAYAPPEND:
        case Builtin::ADD:
        case Builtin::SUBTRACT:
        case Builtin::MULTIPLY:
        case Builtin::DIVIDE:
        case Builtin::MODULO:
        case Builtin::COMPARE:
        case Builtin::LENGTH:
        case Builtin::NTH:
        case Builtin::STREAMTOARRAY:
        case Builtin::REDUCE:
        case Builtin::ALL:
        case Builtin::GROUPEDMAPREDUCE:
        case Builtin::ANY: {
            boost::shared_ptr<scoped_cJSON_t> arr = eval_call_as_json(c, env, scopes, backtrace);
            require_type(arr->get(), cJSON_Array, backtrace);
            json_array_iterator_t it(arr->get());
            return boost::shared_ptr<json_stream_t>(new in_memory_stream_t(it));
        } break;
        case Builtin::FILTER:
            {
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
                return stream->add_transformation(c->builtin().filter(), env, scopes, backtrace.with("predicate"));
            }
            break;
        case Builtin::MAP:
            {
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
                return stream->add_transformation(c->builtin().map().mapping(), env, scopes, backtrace.with("mapping"));
            }
            break;
        case Builtin::CONCATMAP:
            {
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
                return stream->add_transformation(c->builtin().concat_map(), env, scopes, backtrace.with("mapping"));
            }
            break;
        case Builtin::ORDERBY: {
            ordering_t o(c->builtin().order_by(), backtrace.with("order_by"));
            boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

            boost::shared_ptr<in_memory_stream_t> sorted_stream(new in_memory_stream_t(stream));
            sorted_stream->sort(o);
            return sorted_stream;
        }
            break;
        case Builtin::DISTINCT:
            {
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                shared_scoped_less_t comparator(backtrace);

                return boost::shared_ptr<json_stream_t>(new distinct_stream_t<shared_scoped_less_t>(stream, comparator));
            }
            break;
        case Builtin::SLICE:
            {
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                int start, stop;
                bool stop_unbounded = false;

                {
                    boost::shared_ptr<scoped_cJSON_t> start_json = eval_term_as_json_and_check_either(c->mutable_args(1), env, scopes, backtrace.with("arg:1"), cJSON_NULL, cJSON_Number, "Slice start must be null or a nonnegative integer.");
                    if (start_json->type() == cJSON_NULL) {
                        start = 0;
                    } else {    // cJSON_Number
                        float float_start = start_json->get()->valuedouble;
                        start = static_cast<int>(float_start);
                        if (float_start != start || start < 0) {
                            throw runtime_exc_t("Slice start must be null or a nonnegative integer.", backtrace.with("arg:1"));
                        }
                    }
                }

                // Check third arg type
                {
                    boost::shared_ptr<scoped_cJSON_t> stop_json = eval_term_as_json_and_check_either(c->mutable_args(2), env, scopes, backtrace.with("arg:2"), cJSON_NULL, cJSON_Number, "Slice stop must be null or a nonnegative integer.");
                    if (stop_json->type() == cJSON_NULL) {
                        stop_unbounded = true;
                        stop = 0;
                    } else {
                        float float_stop = stop_json->get()->valuedouble;
                        stop = static_cast<int>(float_stop);
                        if (float_stop != stop || stop < 0) {
                            throw runtime_exc_t("Slice stop must be null or a nonnegative integer.", backtrace.with("arg:2"));
                        }
                    }
                }

                if (!stop_unbounded && stop < start) {
                    throw runtime_exc_t("Slice stop cannot be before slice start", backtrace.with("arg:2"));
                }

                return boost::shared_ptr<json_stream_t>(new slice_stream_t(stream, start, stop_unbounded, stop));
            }
        case Builtin::UNION:
            {
                union_stream_t::stream_list_t streams;

                streams.push_back(eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0")));

                streams.push_back(eval_term_as_stream(c->mutable_args(1), env, scopes, backtrace.with("arg:1")));

                return boost::shared_ptr<json_stream_t>(new union_stream_t(streams));
            }
            break;
        case Builtin::ARRAYTOSTREAM:
            {
                boost::shared_ptr<scoped_cJSON_t> array = eval_term_as_json(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
                require_type(array->get(), cJSON_Array, backtrace);
                json_array_iterator_t it(array->get());

                return boost::shared_ptr<json_stream_t>(new in_memory_stream_t(it));
            }
            break;
        case Builtin::RANGE:
            {
                // TODO: ***use an index***
                boost::shared_ptr<json_stream_t> stream = eval_term_as_stream(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                boost::shared_ptr<scoped_cJSON_t> lowerbound, upperbound;

                Builtin::Range *r = c->mutable_builtin()->mutable_range();

                key_range_t range;

                if (r->has_lowerbound()) {
                    lowerbound = eval_term_as_json(r->mutable_lowerbound(), env, scopes, backtrace.with("lowerbound"));
                    if (lowerbound->type() != cJSON_Number && lowerbound->type() != cJSON_String) {
                        throw runtime_exc_t(strprintf("Lower bound of RANGE must be a string or a number, not %s.",
                                                      lowerbound->Print().c_str()), backtrace.with("lowerbound"));
                    }
                }

                if (r->has_upperbound()) {
                    upperbound = eval_term_as_json(r->mutable_upperbound(), env, scopes, backtrace.with("upperbound"));
                    if (upperbound->type() != cJSON_Number && upperbound->type() != cJSON_String) {
                        throw runtime_exc_t(strprintf("Lower bound of RANGE must be a string or a number, not %s.",
                                                      upperbound->Print().c_str()), backtrace.with("upperbound"));
                    }
                }

                if (lowerbound && upperbound) {
                    if (cJSON_cmp(lowerbound->get(), upperbound->get(), backtrace) >= 0) {
                        throw runtime_exc_t(strprintf("Lower bound of RANGE must be <= upper bound (%s vs. %s).",
                                                      lowerbound->Print().c_str(), upperbound->Print().c_str()),
                                            backtrace.with("lowerbound"));
                    }

                    range = key_range_t(key_range_t::closed, store_key_t(cJSON_print_primary(lowerbound->get(), backtrace)),
                                        key_range_t::closed, store_key_t(cJSON_print_primary(upperbound->get(), backtrace)));
                } else if (lowerbound) {
                    range = key_range_t(key_range_t::closed, store_key_t(cJSON_print_primary(lowerbound->get(), backtrace)),
                                        key_range_t::none, store_key_t());
                } else if (upperbound) {
                    range = key_range_t(key_range_t::none, store_key_t(),
                                        key_range_t::closed, store_key_t(cJSON_print_primary(upperbound->get(), backtrace)));
                }

                return boost::shared_ptr<json_stream_t>(
                    new range_stream_t(stream, range, r->attrname(), backtrace));
            }
            //TODO: wtf is this still doing here?
            throw runtime_exc_t("Unimplemented: Builtin::RANGE", backtrace);
            break;
        default:
            crash("unreachable");
            break;
    }
    crash("unreachable");

}

namespace_repo_t<rdb_protocol_t>::access_t eval_table_ref(TableRef *t, runtime_environment_t *env, const backtrace_t &bt)
    THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    // TODO(1253): push name_string_t up?
    std::string table_name_str = t->table_name();
    name_string_t table_name;
    if (!table_name.assign(table_name_str)) {
        throw runtime_exc_t("invalid table name: " + table_name_str, bt);  // TODO(1253) duplicate message, DRY
    }
    std::string db_name = t->db_name();

    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> > namespaces_metadata = env->namespaces_semilattice_metadata->get();
    databases_semilattice_metadata_t databases_metadata = env->databases_semilattice_metadata->get();

    cow_ptr_t<namespaces_semilattice_metadata_t<rdb_protocol_t> >::change_t namespaces_metadata_change(&namespaces_metadata);
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
        ns_searcher(&namespaces_metadata_change.get()->namespaces);
    metadata_searcher_t<database_semilattice_metadata_t>
        db_searcher(&databases_metadata.databases);

    uuid_t db_id = meta_get_uuid(db_searcher, db_name, "EVAL_DB " + db_name, bt);
    namespace_predicate_t pred(&table_name, &db_id);
    uuid_t id = meta_get_uuid(ns_searcher, pred, "EVAL_TABLE " + table_name_str, bt);

    return namespace_repo_t<rdb_protocol_t>::access_t(env->ns_repo, id, env->interruptor);
}

view_t eval_call_as_view(Term::Call *c, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    switch (c->builtin().type()) {
        //JSON -> JSON
        case Builtin::NOT:
        case Builtin::GETATTR:
        case Builtin::IMPLICIT_GETATTR:
        case Builtin::HASATTR:
        case Builtin::IMPLICIT_HASATTR:
        case Builtin::WITHOUT:
        case Builtin::IMPLICIT_WITHOUT:
        case Builtin::PICKATTRS:
        case Builtin::IMPLICIT_PICKATTRS:
        case Builtin::MAPMERGE:
        case Builtin::ARRAYAPPEND:
        case Builtin::ADD:
        case Builtin::SUBTRACT:
        case Builtin::MULTIPLY:
        case Builtin::DIVIDE:
        case Builtin::MODULO:
        case Builtin::COMPARE:
        case Builtin::LENGTH:
        case Builtin::NTH:
        case Builtin::STREAMTOARRAY:
        case Builtin::REDUCE:
        case Builtin::ALL:
        case Builtin::ANY:
        case Builtin::GROUPEDMAPREDUCE:
        case Builtin::MAP:
        case Builtin::CONCATMAP:
        case Builtin::DISTINCT:
        case Builtin::UNION:
        case Builtin::ARRAYTOSTREAM:
            unreachable("eval_call_as_view called on a function that does not return a view");
            break;
        case Builtin::FILTER:
            {
                view_t view = eval_term_as_view(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));
                boost::shared_ptr<json_stream_t> new_stream =
                    view.stream->add_transformation(c->builtin().filter(), env, scopes, backtrace.with("predicate"));
                return view_t(view.access, view.primary_key, new_stream);
            }
            break;
        case Builtin::ORDERBY:
            {
                ordering_t o(c->builtin().order_by(), backtrace.with("order_by"));
                view_t view = eval_term_as_view(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                boost::shared_ptr<in_memory_stream_t> sorted_stream(new in_memory_stream_t(view.stream));
                sorted_stream->sort(o);
                return view_t(view.access, view.primary_key, sorted_stream);
            }
            break;
        case Builtin::SLICE:
            {
                view_t view = eval_term_as_view(c->mutable_args(0), env, scopes, backtrace.with("arg:0"));

                int start, stop;
                bool stop_unbounded = false;

                {
                    boost::shared_ptr<scoped_cJSON_t> start_json = eval_term_as_json_and_check_either(c->mutable_args(1), env, scopes, backtrace.with("arg:1"), cJSON_NULL, cJSON_Number, "Slice start must be null or a nonnegative integer.");
                    if (start_json->type() == cJSON_NULL) {
                        start = 0;
                    } else {    // cJSON_Number
                        float float_start = start_json->get()->valuedouble;
                        start = static_cast<int>(float_start);
                        if (float_start != start || start < 0) {
                            throw runtime_exc_t("Slice start must be null or a nonnegative integer.", backtrace.with("arg:1"));
                        }
                    }
                }

                // Check third arg type
                {
                    boost::shared_ptr<scoped_cJSON_t> stop_json = eval_term_as_json_and_check_either(c->mutable_args(2), env, scopes, backtrace.with("arg:2"), cJSON_NULL, cJSON_Number, "Slice stop must be null or a nonnegative integer.");
                    if (stop_json->type() == cJSON_NULL) {
                        stop_unbounded = true;
                        stop = 0;
                    } else {
                        float float_stop = stop_json->get()->valuedouble;
                        stop = static_cast<int>(float_stop);
                        if (float_stop != stop || stop < 0) {
                            throw runtime_exc_t("Slice stop must be null or a nonnegative integer.", backtrace.with("arg:2"));
                        }
                    }
                }

                if (!stop_unbounded && stop < start) {
                    throw runtime_exc_t("Slice stop cannot be before slice start", backtrace.with("arg:2"));
                }

                return view_t(view.access, view.primary_key, boost::shared_ptr<json_stream_t>(new slice_stream_t(view.stream, start, stop_unbounded, stop)));
            }
            break;
        case Builtin::RANGE:
            throw runtime_exc_t("Unimplemented: Builtin::RANGE", backtrace);
            break;
        default:
            crash("unreachable");
            break;
    }
    crash("unreachable");

}

view_t eval_term_as_view(Term *t, runtime_environment_t *env, const scopes_t &scopes, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    switch (t->type()) {
    case Term::IMPLICIT_VAR:
    case Term::VAR:
    case Term::LET:
    case Term::ERROR:
    case Term::NUMBER:
    case Term::STRING:
    case Term::JSON:
    case Term::BOOL:
    case Term::JSON_NULL:
    case Term::ARRAY:
    case Term::OBJECT:
    case Term::JAVASCRIPT:
        crash("eval_term_as_view called on incompatible Term");
        break;
    case Term::CALL:
        return eval_call_as_view(t->mutable_call(), env, scopes, backtrace);
        break;
    case Term::IF:
        {
            boost::shared_ptr<scoped_cJSON_t> test = eval_term_as_json_and_check_either(t->mutable_if_()->mutable_test(), env, scopes, backtrace.with("test"), cJSON_True, cJSON_False, "The IF test must evaluate to a boolean.");

            if (test->type() == cJSON_True) {
                return eval_term_as_view(t->mutable_if_()->mutable_true_branch(), env, scopes, backtrace.with("true"));
            } else {
                return eval_term_as_view(t->mutable_if_()->mutable_false_branch(), env, scopes, backtrace.with("false"));
            }
        }
        break;
    case Term::GETBYKEY:
        crash("unimplemented");
        break;
    case Term::TABLE:
        return eval_table_as_view(t->mutable_table(), env, backtrace.with("table_ref"));
        break;
    default:
        unreachable();
    }
    unreachable();
}

view_t eval_table_as_view(Term::Table *t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(interrupted_exc_t, runtime_exc_t, broken_client_exc_t) {
    namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval_table_ref(t->mutable_table_ref(), env, backtrace);
    std::string pk = get_primary_key(t->mutable_table_ref(), env, backtrace);
    boost::shared_ptr<json_stream_t> stream(new batched_rget_stream_t(ns_access, env->interruptor, key_range_t::universe(), 100, backtrace, t->table_ref().use_outdated()));
    return view_t(ns_access, pk, stream);
}

} //namespace query_language

#undef guarantee_debug_throw_release
