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

namespace query_language {

cJSON *safe_cJSON_CreateNumber(double d, const backtrace_t &backtrace) {
    if (!isfinite(d)) throw runtime_exc_t(strprintf("Illegal numeric value %e.", d), backtrace);
    return cJSON_CreateNumber(d);
}

/* Convenience function for making the horrible easy. */
boost::shared_ptr<scoped_cJSON_t> shared_scoped_json(cJSON *json) {
    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(json));
}


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

term_info_t get_term_type(const Term &t, type_checking_environment_t *env, const backtrace_t &backtrace) {
    check_protobuf(Term::TermType_IsValid(t.type()));

    std::vector<const google::protobuf::FieldDescriptor *> fields;
    t.GetReflection()->ListFields(t, &fields);
    int field_count = fields.size();

    check_protobuf(field_count <= 2);

    switch (t.type()) {
    case Term::VAR:
        check_protobuf(t.has_var());
        if (!env->scope.is_in_scope(t.var())) {
            throw bad_query_exc_t(strprintf("symbol '%s' is not in scope", t.var().c_str()), backtrace);
        }
        return env->scope.get(t.var());
        break;
    case Term::LET:
        {
            check_protobuf(t.has_let());
            new_scope_t scope_maker(&env->scope); //create a new scope
            for (int i = 0; i < t.let().binds_size(); ++i) {
                env->scope.put_in_scope(
                    t.let().binds(i).var(),
                    get_term_type(
                        t.let().binds(i).term(),
                        env,
                        backtrace.with(strprintf("bind:%s", t.let().binds(i).var().c_str()))));
            }
            term_info_t res = get_term_type(t.let().expr(), env, backtrace.with("expr"));
            return res;
        }
        break;
    case Term::CALL:
        check_protobuf(t.has_call());
        return get_function_type(t.call(), env, backtrace);
        break;
    case Term::IF:
        {
            check_protobuf(t.has_if_());
            bool test_is_det;
            check_term_type(t.if_().test(), TERM_TYPE_JSON, env, &test_is_det, backtrace.with("test"));

            term_info_t true_branch = get_term_type(t.if_().true_branch(), env, backtrace.with("true_branch"));

            term_info_t false_branch = get_term_type(t.if_().false_branch(), env, backtrace.with("false_branch"));

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
            return combined_type;
        }
        break;
    case Term::ERROR:
        check_protobuf(t.has_error());
        return term_info_t(TERM_TYPE_ARBITRARY, true);
        break;
    case Term::NUMBER:
        check_protobuf(t.has_number());
        return term_info_t(TERM_TYPE_JSON, true);
        break;
    case Term::STRING:
        check_protobuf(t.has_valuestring());
        return term_info_t(TERM_TYPE_JSON, true);
        break;
    case Term::JSON:
        check_protobuf(t.has_jsonstring());
        return term_info_t(TERM_TYPE_JSON, true);
        break;
    case Term::BOOL:
        check_protobuf(t.has_valuebool());
        return term_info_t(TERM_TYPE_JSON, true);
        break;
    case Term::JSON_NULL:
        check_protobuf(field_count == 1); // null term has only a type field
        return term_info_t(TERM_TYPE_JSON, true);
        break;
    case Term::ARRAY:
        {
            if (t.array_size() == 0) { // empty arrays are valid
                check_protobuf(field_count == 1);
            }
            bool deterministic = true;
            for (int i = 0; i < t.array_size(); ++i) {
                check_term_type(t.array(i), TERM_TYPE_JSON, env, &deterministic, backtrace.with(strprintf("elem:%d", i)));
            }
            return term_info_t(TERM_TYPE_JSON, deterministic);
        }
        break;
    case Term::OBJECT:
        {
            if (t.object_size() == 0) { // empty objects are valid
                check_protobuf(field_count == 1);
            }
            bool deterministic = true;
            for (int i = 0; i < t.object_size(); ++i) {
                check_term_type(t.object(i).term(), TERM_TYPE_JSON, env, &deterministic,
                        backtrace.with(strprintf("key:%s", t.object(i).var().c_str())));
            }
            return term_info_t(TERM_TYPE_JSON, deterministic);
        }
        break;
    case Term::GETBYKEY: {
        check_protobuf(t.has_get_by_key());
        check_term_type(t.get_by_key().key(), TERM_TYPE_JSON, env, NULL, backtrace.with("key"));
        return term_info_t(TERM_TYPE_JSON, false);
        break;
    }
    case Term::TABLE:
        check_protobuf(t.has_table());
        return term_info_t(TERM_TYPE_VIEW, false);
        break;
    case Term::JAVASCRIPT:
        check_protobuf(t.has_javascript());
        return term_info_t(TERM_TYPE_JSON, true); //javascript is never deterministic
        break;
    default:
        unreachable("unhandled Term case");
    }
    crash("unreachable");
}

void check_term_type(const Term &t, term_type_t expected, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    term_info_t actual = get_term_type(t, env, backtrace);
    if (!term_type_is_convertible(actual.type, expected)) {
        throw bad_query_exc_t(strprintf("expected a %s; got a %s",
                term_type_name(expected).c_str(), term_type_name(actual.type).c_str()),
            backtrace);
    }

    if (is_det_out) {
        *is_det_out &= actual.deterministic;
    }
}

void check_arg_count(const Term::Call &c, int n_args, const backtrace_t &backtrace) {
    if (c.args_size() != n_args) {
        const char* fn_name = Builtin::BuiltinType_Name(c.builtin().type()).c_str();
        throw bad_query_exc_t(strprintf(
            "%s takes %d argument%s (%d given)",
            fn_name,
            n_args,
            n_args != 1 ? "s" : "",
            c.args_size()
            ),
            backtrace);
    }
}

void check_arg_count_at_least(const Term::Call &c, int n_args, const backtrace_t &backtrace) {
    if (c.args_size() < n_args) {
        const char* fn_name = Builtin::BuiltinType_Name(c.builtin().type()).c_str();
        throw bad_query_exc_t(strprintf(
            "%s takes at least %d argument%s (%d given)",
            fn_name,
            n_args,
            n_args != 1 ? "s" : "",
            c.args_size()
            ),
            backtrace);
    }
}

void check_function_args(const Term::Call &c, const term_type_t &arg_type, int n_args,
                         type_checking_environment_t *env, bool *is_det_out,
                         const backtrace_t &backtrace) {
    check_arg_count(c, n_args, backtrace);
    for (int i = 0; i < c.args_size(); ++i) {
        check_term_type(c.args(i), arg_type, env, is_det_out, backtrace.with(strprintf("arg:%d", i)));
    }
}

void check_function_args_at_least(const Term::Call &c, const term_type_t &arg_type, int n_args,
                                  type_checking_environment_t *env, bool *is_det_out,
                                  const backtrace_t &backtrace) {
    // requires the number of arguments to be at least n_args
    check_arg_count_at_least(c, n_args, backtrace);
    for (int i = 0; i < c.args_size(); ++i) {
        check_term_type(c.args(i), arg_type, env, is_det_out, backtrace.with(strprintf("arg:%d", i)));
    }
}

void check_polymorphic_function_args(const Term::Call &c, const term_type_t &arg_type, int n_args,
                         type_checking_environment_t *env, bool *is_det_out,
                         const backtrace_t &backtrace) {
    // for functions that work with selections/streams/jsons, where the first argument can be any type
    check_arg_count(c, n_args, backtrace);
    for (int i = 1; i < c.args_size(); ++i) {
        check_term_type(c.args(i), arg_type, env, is_det_out, backtrace.with(strprintf("arg:%d", i)));
    }
}

void check_function_args(const Term::Call &c, const term_type_t &arg1_type, const term_type_t &arg2_type,
                         type_checking_environment_t *env, bool *is_det_out,
                         const backtrace_t &backtrace) {
    check_arg_count(c, 2, backtrace);
    check_term_type(c.args(0), arg1_type, env, is_det_out, backtrace.with("arg:0"));
    check_term_type(c.args(1), arg2_type, env, is_det_out, backtrace.with("arg:1"));
}

term_info_t get_function_type(const Term::Call &c, type_checking_environment_t *env, const backtrace_t &backtrace) {
    const Builtin &b = c.builtin();

    check_protobuf(Builtin::BuiltinType_IsValid(b.type()));

    bool deterministic = true;
    std::vector<const google::protobuf::FieldDescriptor *> fields;


    b.GetReflection()->ListFields(b, &fields);

    int field_count = fields.size();

    check_protobuf(field_count <= 2);

    // this is a bit cleaner when we check well-formedness separate
    // from returning the type

    switch (c.builtin().type()) {
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
        check_protobuf(b.has_comparison());
        check_protobuf(Builtin::Comparison_IsValid(b.comparison()));
        break;
    case Builtin::GETATTR:
    case Builtin::IMPLICIT_GETATTR:
    case Builtin::HASATTR:
    case Builtin::IMPLICIT_HASATTR:
        check_protobuf(b.has_attr());
        break;
    case Builtin::WITHOUT:
    case Builtin::IMPLICIT_WITHOUT:
    case Builtin::PICKATTRS:
    case Builtin::IMPLICIT_PICKATTRS:
        check_protobuf(b.attrs_size());
        break;
    case Builtin::FILTER: {
        check_protobuf(b.has_filter());
        break;
    }
    case Builtin::MAP: {
        check_protobuf(b.has_map());
        break;
    }
    case Builtin::CONCATMAP: {
        check_protobuf(b.has_concat_map());
        break;
    }
    case Builtin::ORDERBY:
        check_protobuf(b.order_by_size() > 0);
        break;
    case Builtin::REDUCE: {
        //implicit_value_t<term_type_t>::impliciter_t impliciter(&env->implicit_type, TERM_TYPE_JSON); //make the implicit value be of type json
        check_protobuf(b.has_reduce());
        //check_reduction_type(b.reduce(), env, backtrace.with("reduce"));
        break;
    }
    case Builtin::GROUPEDMAPREDUCE: {
        check_protobuf(b.has_grouped_map_reduce());
        break;
    }
    case Builtin::RANGE:
        check_protobuf(b.has_range());
        if (b.range().has_lowerbound()) {
            check_term_type(b.range().lowerbound(), TERM_TYPE_JSON, env, &deterministic, backtrace.with("lowerbound"));
        }
        if (b.range().has_upperbound()) {
            check_term_type(b.range().upperbound(), TERM_TYPE_JSON, env, &deterministic, backtrace.with("upperbound"));
        }
        break;
    default:
        crash("unreachable");
    }

    switch (b.type()) {
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
            check_function_args(c, TERM_TYPE_JSON, c.args_size(), env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_JSON, deterministic);  // variadic JSON type
            break;
        case Builtin::SLICE:
            {
                check_polymorphic_function_args(c, TERM_TYPE_JSON, 3, env, &deterministic, backtrace);
                // polymorphic
                term_info_t res = get_term_type(c.args(0), env, backtrace);
                res.deterministic &= deterministic;
                return res;
            }
        case Builtin::MAP:
            {
                check_arg_count(c, 1, backtrace);

                implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                check_mapping_type(b.map().mapping(), TERM_TYPE_JSON, env, &deterministic, deterministic, backtrace.with("mapping"));
                return term_info_t(TERM_TYPE_STREAM, deterministic);
            }
            break;
        case Builtin::CONCATMAP:
            {
                check_arg_count(c, 1, backtrace);

                implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                //check_mapping_type(b.concat_map().mapping(), TERM_TYPE_STREAM, env, &deterministic, deterministic, backtrace.with("mapping"));
                return term_info_t(TERM_TYPE_STREAM, deterministic);
            }
            break;
        case Builtin::DISTINCT:
            {
                check_arg_count(c, 1, backtrace);

                return term_info_t(TERM_TYPE_STREAM, deterministic);
            }
            break;
        case Builtin::FILTER:
            {
                check_arg_count(c, 1, backtrace);
                // polymorphic
                implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json

                check_predicate_type(b.filter().predicate(), env, &deterministic, deterministic, backtrace.with("predicate"));
                term_info_t res = get_term_type(c.args(0), env, backtrace);
                res.deterministic &= deterministic;
                return res;
            }
            break;
        case Builtin::ORDERBY:
            {
                check_arg_count(c, 1, backtrace);

                term_info_t res = get_term_type(c.args(0), env, backtrace);
                res.deterministic &= deterministic;
                return res;
            }
            break;
        case Builtin::NTH:
            {
                check_polymorphic_function_args(c, TERM_TYPE_JSON, 2, env, &deterministic, backtrace);
                term_info_t res = get_term_type(c.args(0), env, backtrace);
                const_cast<Term::Call&>(c).mutable_args(0)->SetExtension(extension::inferred_type, static_cast<int32_t>(res.type));
                return term_info_t(TERM_TYPE_JSON, deterministic);
                break;
            }
            break;
        case Builtin::LENGTH:
            {
                check_arg_count(c, 1, backtrace);
                term_info_t res = get_term_type(c.args(0), env, backtrace);
                const_cast<Term::Call&>(c).mutable_args(0)->SetExtension(extension::inferred_type, static_cast<int32_t>(res.type));
                return term_info_t(TERM_TYPE_JSON, deterministic & res.deterministic);
            }
        case Builtin::STREAMTOARRAY:
            check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_JSON, deterministic);
            break;
        case Builtin::REDUCE:
            {
                check_arg_count(c, 1, backtrace);
                implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                check_reduction_type(b.reduce(), env, &deterministic, deterministic, backtrace.with("reduce"));
                return term_info_t(TERM_TYPE_JSON, false); //This is always false because we can't be sure the functions is associative or commutative
            }
            break;
        case Builtin::GROUPEDMAPREDUCE:
            {
                check_arg_count(c, 1, backtrace);
                implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                check_mapping_type(b.grouped_map_reduce().group_mapping(), TERM_TYPE_JSON, env, &deterministic, deterministic, backtrace.with("group_mapping"));
                check_mapping_type(b.grouped_map_reduce().value_mapping(), TERM_TYPE_JSON, env, &deterministic, deterministic, backtrace.with("value_mapping"));
                check_reduction_type(b.grouped_map_reduce().reduction(), env, &deterministic, deterministic, backtrace.with("reduction"));
                return term_info_t(TERM_TYPE_JSON, false);
            }
            break;
        case Builtin::UNION:
            check_function_args(c, TERM_TYPE_STREAM, 2, env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_STREAM, deterministic);
            break;
        case Builtin::ARRAYTOSTREAM:
            check_function_args(c, TERM_TYPE_JSON, 1, env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_STREAM, deterministic);
            break;
        case Builtin::RANGE:
            check_arg_count(c, 1, backtrace);
            return term_info_t(TERM_TYPE_STREAM, deterministic);
            break;
        default:
            crash("unreachable");
            break;
    }

    crash("unreachable");
}

void check_reduction_type(const Reduction &r, type_checking_environment_t *env, bool *is_det_out, bool args_are_deterministic, const backtrace_t &backtrace) {
    check_term_type(r.base(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("base"));

    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(r.var1(), term_info_t(TERM_TYPE_JSON, args_are_deterministic));
    env->scope.put_in_scope(r.var2(), term_info_t(TERM_TYPE_JSON, args_are_deterministic));
    check_term_type(r.body(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("body"));
}

void check_mapping_type(const Mapping &m, term_type_t return_type, type_checking_environment_t *env, bool *is_det_out, bool args_are_deterministic, const backtrace_t &backtrace) {
    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(m.arg(), term_info_t(TERM_TYPE_JSON, args_are_deterministic));
    check_term_type(m.body(), return_type, env, is_det_out, backtrace);
}

void check_predicate_type(const Predicate &p, type_checking_environment_t *env, bool *is_det_out, bool args_are_deterministic, const backtrace_t &backtrace) {
    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(p.arg(), term_info_t(TERM_TYPE_JSON, args_are_deterministic));
    check_term_type(p.body(), TERM_TYPE_JSON, env, is_det_out, backtrace);
}

void check_read_query_type(const ReadQuery &rq, type_checking_environment_t *env, bool *, const backtrace_t &backtrace) {
    /* Read queries could return anything--a view, a stream, a JSON, or an
    error. Views will be automatically converted to streams at evaluation time.
    */
    term_info_t res = get_term_type(rq.term(), env, backtrace);
    const_cast<ReadQuery&>(rq).SetExtension(extension::inferred_read_type, static_cast<int32_t>(res.type));
}

void check_write_query_type(const WriteQuery &w, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    check_protobuf(WriteQuery::WriteQueryType_IsValid(w.type()));

    std::vector<const google::protobuf::FieldDescriptor *> fields;
    w.GetReflection()->ListFields(w, &fields);
    check_protobuf(fields.size() == 2);

    bool deterministic = true;
    switch (w.type()) {
        case WriteQuery::UPDATE: {
            check_protobuf(w.has_update());
            check_term_type(w.update().view(), TERM_TYPE_VIEW, env, is_det_out, backtrace.with("view"));
            implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic));
            check_mapping_type(w.update().mapping(), TERM_TYPE_JSON, env, is_det_out, *is_det_out, backtrace.with("mapping"));
            break;
        }
        case WriteQuery::DELETE: {
            check_protobuf(w.has_delete_());
            check_term_type(w.delete_().view(), TERM_TYPE_VIEW, env, is_det_out, backtrace.with("view"));
            break;
        }
        case WriteQuery::MUTATE: {
            check_protobuf(w.has_mutate());
            check_term_type(w.mutate().view(), TERM_TYPE_VIEW, env, is_det_out, backtrace.with("view"));
            implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic));
            check_mapping_type(w.mutate().mapping(), TERM_TYPE_JSON, env, is_det_out, *is_det_out, backtrace.with("mapping"));
            break;
        }
        case WriteQuery::INSERT: {
            check_protobuf(w.has_insert());
            if (w.insert().terms_size() == 1) {
                term_info_t res = get_term_type(w.insert().terms(0), env, backtrace);
                //TODO: This casting is copy-pasted from NTH, but WTF?
                const_cast<WriteQuery&>(w).mutable_insert()->mutable_terms(0)->
                    SetExtension(extension::inferred_type,
                                 static_cast<int32_t>(res.type));
                break; //Single-element insert polymorphic over streams and arrays
            }
            for (int i = 0; i < w.insert().terms_size(); ++i) {
                check_term_type(w.insert().terms(i), TERM_TYPE_JSON, env, is_det_out, backtrace.with(strprintf("term:%d", i)));
            }
            break;
        }
        case WriteQuery::INSERTSTREAM: {
            check_protobuf(w.has_insert_stream());
            check_term_type(w.insert_stream().stream(), TERM_TYPE_STREAM, env, is_det_out, backtrace.with("stream"));
            break;
        }
        case WriteQuery::FOREACH:
            {
                check_protobuf(w.has_for_each());
                //check_term_type(w.for_each().stream(), TERM_TYPE_STREAM, env, is_det_out, backtrace.with("stream"));

                new_scope_t scope_maker(&env->scope, w.for_each().var(), term_info_t(TERM_TYPE_JSON, *is_det_out));
                for (int i = 0; i < w.for_each().queries_size(); ++i) {
                    check_write_query_type(w.for_each().queries(i), env, is_det_out, backtrace.with(strprintf("query:%d", i)));
                }
            }
            break;
        case WriteQuery::POINTUPDATE: {
            check_protobuf(w.has_point_update());
            check_term_type(w.point_update().key(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("key"));
            implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic));
            check_mapping_type(w.point_update().mapping(), TERM_TYPE_JSON, env, is_det_out, *is_det_out, backtrace.with("mapping"));
            break;
        }
        case WriteQuery::POINTDELETE: {
            check_protobuf(w.has_point_delete());
            check_term_type(w.point_delete().key(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("key"));
            break;
        }
        case WriteQuery::POINTMUTATE: {
            check_protobuf(w.has_point_mutate());
            check_term_type(w.point_mutate().key(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("key"));
            implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic));
            check_mapping_type(w.point_mutate().mapping(), TERM_TYPE_JSON, env, is_det_out, *is_det_out, backtrace.with("mapping"));
            break;
        }
        default:
            unreachable("unhandled WriteQuery");
    }
}

void check_meta_query_type(const MetaQuery &t) {
    check_protobuf(MetaQuery::MetaQueryType_IsValid(t.type()));
    switch(t.type()) {
    case MetaQuery::CREATE_DB:
        check_protobuf(t.has_db_name());
        check_protobuf(!t.has_create_table());
        check_protobuf(!t.has_drop_table());
        break;
    case MetaQuery::DROP_DB:
        check_protobuf(t.has_db_name());
        check_protobuf(!t.has_create_table());
        check_protobuf(!t.has_drop_table());
        break;
    case MetaQuery::LIST_DBS:
        check_protobuf(!t.has_db_name());
        check_protobuf(!t.has_create_table());
        check_protobuf(!t.has_drop_table());
        break;
    case MetaQuery::CREATE_TABLE:
        check_protobuf(!t.has_db_name());
        check_protobuf(t.has_create_table());
        check_protobuf(!t.has_drop_table());
        break;
    case MetaQuery::DROP_TABLE:
        check_protobuf(!t.has_db_name());
        check_protobuf(!t.has_create_table());
        check_protobuf(t.has_drop_table());
        break;
    case MetaQuery::LIST_TABLES:
        check_protobuf(t.has_db_name());
        check_protobuf(!t.has_create_table());
        check_protobuf(!t.has_drop_table());
        break;
    default: unreachable("Unhandled MetaQuery.");
    }
}

void check_query_type(const Query &q, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    check_protobuf(Query::QueryType_IsValid(q.type()));
    switch (q.type()) {
    case Query::READ:
        check_protobuf(q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_protobuf(!q.has_meta_query());
        check_read_query_type(q.read_query(), env, is_det_out, backtrace);
        break;
    case Query::WRITE:
        check_protobuf(q.has_write_query());
        check_protobuf(!q.has_read_query());
        check_protobuf(!q.has_meta_query());
        check_write_query_type(q.write_query(), env, is_det_out, backtrace);
        break;
    case Query::CONTINUE:
        check_protobuf(!q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_protobuf(!q.has_meta_query());
        break;
    case Query::STOP:
        check_protobuf(!q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_protobuf(!q.has_meta_query());
        break;
    case Query::META:
        check_protobuf(q.has_meta_query());
        check_protobuf(!q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_meta_query_type(q.meta_query());
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

class namespace_predicate_t {
public:
    bool operator()(namespace_semilattice_metadata_t<rdb_protocol_t> ns) {
        if (name && (ns.name.in_conflict() || ns.name.get() != *name)) {
            return false;
        } else if (db_id && (ns.database.in_conflict() || ns.database.get() != *db_id)) {
            return false;
        }
        return true;
    }
    explicit namespace_predicate_t(std::string &_name): name(&_name), db_id(0) { }
    explicit namespace_predicate_t(uuid_t &_db_id): name(0), db_id(&_db_id) { }
    namespace_predicate_t(std::string &_name, uuid_t &_db_id):
        name(&_name), db_id(&_db_id) { }
private:
    const std::string *name;
    const uuid_t *db_id;
};

static void meta_check(const char *status, const char *want,
                       const std::string &operation, const backtrace_t &backtrace) {
    if (status != want) {
        if (status == METADATA_SUCCESS) status = "Entry already exists.";
        throw runtime_exc_t(strprintf("Error during operation `%s`: %s",
                                      operation.c_str(), status), backtrace);
    }
}

template<class T, class U>
static uuid_t meta_get_uuid(T searcher, U predicate,
                     const std::string &operation, const backtrace_t &backtrace) {
    const char *status;
    typename T::iterator entry = searcher.find_uniq(predicate, &status);
    meta_check(status, METADATA_SUCCESS, operation, backtrace);
    return entry->first;
}

void execute_meta(MetaQuery *m, runtime_environment_t *env, Response *res, const backtrace_t &bt) THROWS_ONLY(runtime_exc_t, broken_client_exc_t) {
    cluster_semilattice_metadata_t metadata = env->semilattice_metadata->get();
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
        ns_searcher(&metadata.rdb_namespaces.namespaces);
    metadata_searcher_t<database_semilattice_metadata_t>
        db_searcher(&metadata.databases.databases);
    metadata_searcher_t<datacenter_semilattice_metadata_t>
        dc_searcher(&metadata.datacenters.datacenters);

    const char *status;
    switch(m->type()) {
    case MetaQuery::CREATE_DB: {
        std::string db_name = m->db_name();

        /* Ensure database doesn't already exist. */
        db_searcher.find_uniq(db_name, &status);
        meta_check(status, METADATA_ERR_NONE, "CREATE_DB "+db_name, bt);

        /* Create namespace, insert into metadata, then join into real metadata. */
        database_semilattice_metadata_t db;
        db.name = vclock_t<std::string>(db_name, env->this_machine);
        metadata.databases.databases.insert(std::make_pair(generate_uuid(), db));
        env->semilattice_metadata->join(metadata);
        res->set_status_code(Response::SUCCESS_EMPTY); //return immediately.
    } break;
    case MetaQuery::DROP_DB: {
        std::string db_name = m->db_name();

        // Get database metadata.
        metadata_searcher_t<database_semilattice_metadata_t>::iterator
            db_metadata = db_searcher.find_uniq(db_name, &status);
        meta_check(status, METADATA_SUCCESS, "DROP_DB "+db_name, bt);
        rassert(!db_metadata->second.is_deleted());
        uuid_t db_id = db_metadata->first;

        // Delete all tables in database.
        namespace_predicate_t pred(db_id);
        for (metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
                 ::iterator it = ns_searcher.find_next(ns_searcher.begin(), pred);
             it != ns_searcher.end(); it = ns_searcher.find_next(++it, pred)) {
            rassert(!it->second.is_deleted());
            it->second.mark_deleted();
        }

        // Delete database.
        db_metadata->second.mark_deleted();
        env->semilattice_metadata->join(metadata);
        res->set_status_code(Response::SUCCESS_EMPTY);
    } break;
    case MetaQuery::LIST_DBS: {
        for (metadata_searcher_t<database_semilattice_metadata_t>::iterator
                 it = db_searcher.find_next(db_searcher.begin());
             it != db_searcher.end(); it = db_searcher.find_next(++it)) {

            // For each undeleted and unconflicted entry in the metadata, add a response.
            rassert(!it->second.is_deleted());
            if (it->second.get().name.in_conflict()) continue;
            scoped_cJSON_t json(cJSON_CreateString(it->second.get().name.get().c_str()));
            res->add_response(json.PrintUnformatted());
        }
        res->set_status_code(Response::SUCCESS_STREAM);
    } break;
    case MetaQuery::CREATE_TABLE: {
        std::string dc_name = m->create_table().datacenter();
        std::string db_name = m->create_table().table_ref().db_name();
        std::string table_name = m->create_table().table_ref().table_name();
        std::string primary_key = m->create_table().primary_key();

        uuid_t db_id = meta_get_uuid(db_searcher, db_name, "FIND_DATABASE "+db_name, bt);
        uuid_t dc_id = meta_get_uuid(dc_searcher, dc_name,"FIND_DATACENTER "+dc_name,bt);

        /* Ensure table doesn't already exist. */
        ns_searcher.find_uniq(namespace_predicate_t(table_name, db_id), &status);
        meta_check(status, METADATA_ERR_NONE, "CREATE_TABLE "+table_name, bt);

        /* Create namespace, insert into metadata, then join into real metadata. */
        uuid_t namespace_id = generate_uuid();
        //TODO(mlucy): What is the port for?  Why is it always the same?
        namespace_semilattice_metadata_t<rdb_protocol_t> ns =
            new_namespace<rdb_protocol_t>(env->this_machine, db_id, dc_id, table_name,
                                          primary_key, port_constants::namespace_port);
        metadata.rdb_namespaces.namespaces.insert(std::make_pair(namespace_id, ns));
        fill_in_blueprints(&metadata, env->directory_metadata->get(), env->this_machine);
        env->semilattice_metadata->join(metadata);

        /* The following is an ugly hack, but it's probably what we want.  It
           takes about a third of a second for the new namespace to get to the
           point where we can do reads/writes on it.  We don't want to return
           until that has happened, so we try to do a read every `poll_ms`
           milliseconds until one succeeds, then return. */

        int64_t poll_ms = 100; //with this value, usually polls twice
        //This read won't succeed, but we care whether it fails with an exception.
        rdb_protocol_t::read_t bad_read(rdb_protocol_t::point_read_t(store_key_t("")));
        try {
            for (;;) {
                signal_timer_t start_poll(poll_ms);
                wait_interruptible(&start_poll, env->interruptor);
                try {
                    namespace_repo_t<rdb_protocol_t>::access_t ns_access(env->ns_repo, namespace_id, env->interruptor);
                    rdb_protocol_t::read_response_t read_res;
                    ns_access.get_namespace_if()->read(bad_read, &read_res, order_token_t::ignore, env->interruptor);
                    break;
                } catch (cannot_perform_query_exc_t e) { } //continue loop
            }
        } catch (interrupted_exc_t e) {
            throw runtime_exc_t("Query interrupted, probably by user.", bt);
        }
        res->set_status_code(Response::SUCCESS_EMPTY);
    } break;
    case MetaQuery::DROP_TABLE: {
        std::string db_name = m->drop_table().db_name();
        std::string table_name = m->drop_table().table_name();

        // Get namespace metadata.
        uuid_t db_id = meta_get_uuid(db_searcher, db_name, "FIND_DATABASE "+db_name, bt);
        metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >::iterator
            ns_metadata =
            ns_searcher.find_uniq(namespace_predicate_t(table_name, db_id), &status);
        std::string op=strprintf("FIND_TABLE %s.%s",db_name.c_str(),table_name.c_str());
        meta_check(status, METADATA_SUCCESS, op, bt);
        rassert(!ns_metadata->second.is_deleted());

        // Delete namespace
        ns_metadata->second.mark_deleted();
        env->semilattice_metadata->join(metadata);
        res->set_status_code(Response::SUCCESS_EMPTY); //return immediately
    } break;
    case MetaQuery::LIST_TABLES: {
        std::string db_name = m->db_name();
        uuid_t db_id = meta_get_uuid(db_searcher, db_name, "FIND_DATABASE "+db_name, bt);
        namespace_predicate_t pred(db_id);
        for (metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
                 ::iterator it = ns_searcher.find_next(ns_searcher.begin(), pred);
             it != ns_searcher.end(); it = ns_searcher.find_next(++it, pred)) {

            // For each undeleted and unconflicted entry in the metadata, add a response.
            rassert(!it->second.is_deleted());
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
    const char *status;
    std::string db_name = t->db_name();
    std::string table_name = t->table_name();
    namespaces_semilattice_metadata_t<rdb_protocol_t> ns_metadata = env->namespaces_semilattice_metadata->get();
    databases_semilattice_metadata_t db_metadata = env->databases_semilattice_metadata->get();

    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
        ns_searcher(&ns_metadata.namespaces);
    metadata_searcher_t<database_semilattice_metadata_t>
        db_searcher(&db_metadata.databases);

    uuid_t db_id = meta_get_uuid(db_searcher, db_name, "FIND_DB "+db_name, bt);
    namespace_predicate_t pred(table_name, db_id);
    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >::iterator
        ns_metadata_it = ns_searcher.find_uniq(pred, &status);
    meta_check(status, METADATA_SUCCESS, "FIND_TABLE "+table_name, bt);
    rassert(!ns_metadata_it->second.is_deleted());
    if (ns_metadata_it->second.get().primary_key.in_conflict()) {
        throw runtime_exc_t(strprintf(
            "Table %s.%s has a primary key in conflict, which should never happen.",
            db_name.c_str(), table_name.c_str()), bt);
    }
    return ns_metadata_it->second.get().primary_key.get();
}

void execute(Query *q, runtime_environment_t *env, Response *res, const backtrace_t &backtrace, stream_cache_t *stream_cache) THROWS_ONLY(runtime_exc_t, broken_client_exc_t) {
    rassert(q->token() == res->token());
    switch(q->type()) {
    case Query::READ:
        execute(q->mutable_read_query(), env, res, backtrace, stream_cache);
        break; //status set in [execute]
    case Query::WRITE:
        execute(q->mutable_write_query(), env, res, backtrace);
        break; //status set in [execute]
    case Query::CONTINUE:
        if (!stream_cache->serve(q->token(), res)) {
            throw runtime_exc_t(strprintf("Could not serve key %ld from stream cache.", q->token()), backtrace);
        }
        break; //status set in [serve]
    case Query::STOP:
        if (!stream_cache->contains(q->token())) {
            throw broken_client_exc_t(strprintf("No key %ld in stream cache.", q->token()));
        } else {
            res->set_status_code(Response::SUCCESS_EMPTY);
            stream_cache->erase(q->token());
        }
        break;
    case Query::META:
        execute_meta(q->mutable_meta_query(), env, res, backtrace);
        break;
    default:
        crash("unreachable");
    }
}

void execute(ReadQuery *r, runtime_environment_t *env, Response *res, const backtrace_t &backtrace, stream_cache_t *stream_cache) THROWS_ONLY(runtime_exc_t, broken_client_exc_t) {
    int type = r->GetExtension(extension::inferred_read_type);

    switch (type) {
    case TERM_TYPE_JSON: {
        boost::shared_ptr<scoped_cJSON_t> json = eval(r->mutable_term(), env, backtrace);
        res->add_response(json->PrintUnformatted());
        res->set_status_code(Response::SUCCESS_JSON);
        break;
    }
    case TERM_TYPE_STREAM:
    case TERM_TYPE_VIEW: {
        boost::shared_ptr<json_stream_t> stream = eval_stream(r->mutable_term(), env, backtrace);
        int64_t key = res->token();
        if (stream_cache->contains(key)) {
            throw runtime_exc_t(strprintf("Token %ld already in stream cache, use CONTINUE.", key), backtrace);
        } else {
            stream_cache->insert(r, key, stream);
        }
        DEBUG_VAR bool b = stream_cache->serve(key, res);
        rassert(b);
        break; //status code set in [serve]
    }
    case TERM_TYPE_ARBITRARY: {
        eval(r->mutable_term(), env, backtrace);
        unreachable("This term has type `TERM_TYPE_ARBITRARY`, so evaluating "
            "it should throw `runtime_exc_t`.");
    }
    default:
        unreachable("read query type invalid.");
    }
}

void insert(namespace_repo_t<rdb_protocol_t>::access_t ns_access, const std::string &pk, boost::shared_ptr<scoped_cJSON_t> data, runtime_environment_t *env, const backtrace_t &backtrace) {
    if (!data->GetObjectItem(pk.c_str())) {
        throw runtime_exc_t(strprintf("Must have a field named \"%s\" (The primary key).", pk.c_str()), backtrace);
    }
    cJSON *primary_key = data->GetObjectItem(pk.c_str());
    if (primary_key->type != cJSON_String && primary_key->type != cJSON_Number) {
        throw runtime_exc_t(strprintf("Cannot insert row %s with primary key %s of non-string, non-number type.",
                                      data->Print().c_str(), cJSON_Print_std(primary_key).c_str()), backtrace);
    }

    try {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t(cJSON_print_std_string(data->GetObjectItem(pk.c_str()))), data));
        rdb_protocol_t::write_response_t response;
        ns_access.get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

point_modify::result_t point_modify(namespace_repo_t<rdb_protocol_t>::access_t ns_access,
                                    const std::string &pk, cJSON *id, point_modify::op_t op,
                                    runtime_environment_t *env, const Mapping &m, const backtrace_t &backtrace) {
    try {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_modify_t(pk, store_key_t(cJSON_print_std_string(id)), op, env->scopes, m));
        rdb_protocol_t::write_response_t response;
        ns_access.get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);
        rdb_protocol_t::point_modify_response_t mod_res = boost::get<rdb_protocol_t::point_modify_response_t>(response.response);
        if (mod_res.result == point_modify::ERROR) throw mod_res.exc;
        return mod_res.result;
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, cJSON *id, runtime_environment_t *env, const backtrace_t &backtrace, int *out_num_deletes=0) {
    try {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_delete_t(store_key_t(cJSON_print_std_string(id))));
        rdb_protocol_t::write_response_t response;
        ns_access.get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);
        if (out_num_deletes) {
            if (boost::get<rdb_protocol_t::point_delete_response_t>(response.response).result == DELETED) {
                *out_num_deletes = 1;
            } else {
                *out_num_deletes = 0;
            }
        }
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> id, runtime_environment_t *env, const backtrace_t &backtrace, int *out_num_deletes=0) {
    point_delete(ns_access, id->get(), env, backtrace, out_num_deletes);
}

void execute(WriteQuery *w, runtime_environment_t *env, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t, broken_client_exc_t) {
    //TODO: When writes can return different responses, be more clever.
    res->set_status_code(Response::SUCCESS_JSON);
    switch (w->type()) {
        case WriteQuery::UPDATE:
            {
                view_t view = eval_view(w->mutable_update()->mutable_view(), env, backtrace.with("view"));
                std::string reported_error = "";

                int updated = 0, errors = 0, skipped = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    rassert(json && json->type() == cJSON_Object);
                    try {
                        std::string pk = view.primary_key;
                        cJSON *id = json->GetObjectItem(pk.c_str());
                        point_modify::result_t mres =
                            point_modify(view.access, pk, id, point_modify::UPDATE, env, w->update().mapping(), backtrace);
                        rassert(mres == point_modify::MODIFIED || mres == point_modify::SKIPPED);
                        updated += (mres == point_modify::MODIFIED);
                        skipped += (mres == point_modify::SKIPPED);
                    } catch (const query_language::broken_client_exc_t &e) {
                        ++errors;
                        if (reported_error == "") reported_error = e.message;
                    } catch (const query_language::runtime_exc_t &e) {
                        ++errors;
                        if (reported_error == "") reported_error = e.message + "\n" + e.backtrace.print();
                    }
                }
                std::string res_list = strprintf("\"updated\": %d, \"skipped\": %d, \"errors\": %d", updated, skipped, errors);
                if (reported_error != "") {
                    res_list = strprintf("%s, \"first_error\": %s", res_list.c_str(),
                                         scoped_cJSON_t(cJSON_CreateString(reported_error.c_str())).Print().c_str());
                }
                res->add_response("{"+res_list+"}");
            }
            break;
        case WriteQuery::DELETE:
            {
                view_t view = eval_view(w->mutable_delete_()->mutable_view(), env, backtrace.with("view"));

                int deleted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    point_delete(view.access, json->GetObjectItem(view.primary_key.c_str()), env, backtrace);
                    deleted++;
                }

                res->add_response(strprintf("{\"deleted\": %d}", deleted));
            }
            break;
        case WriteQuery::MUTATE:
            {
                view_t view = eval_view(w->mutable_mutate()->mutable_view(), env, backtrace.with("view"));

                int modified = 0, deleted = 0, errors = 0;
                std::string reported_error = "";
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    try {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scopes.scope, w->mutate().mapping().arg(), json);
                        implicit_value_setter_t impliciter(&env->scopes.implicit_attribute_value, json);
                        boost::shared_ptr<scoped_cJSON_t> val =
                            eval(w->mutable_mutate()->mutable_mapping()->mutable_body(), env, backtrace.with("mapping"));
                        if (val->type() == cJSON_NULL) {
                            point_delete(view.access, json->GetObjectItem(view.primary_key.c_str()), env, backtrace);
                            ++deleted;
                        } else if (val->type() != cJSON_Object) {
                            ++errors;
                            if (reported_error == "") {
                                reported_error = strprintf("Mutate returned a non-object (%s).\n", val->Print().c_str());
                            }
                        } else {
                            cJSON *json_key = json->GetObjectItem(view.primary_key.c_str());
                            cJSON *val_key = val->GetObjectItem(view.primary_key.c_str());
                            if (!cJSON_Equal(json_key, val_key)) {
                                ++errors;
                                if (reported_error == "") {
                                    reported_error = strprintf("Mutate cannot change primary keys: %s -> %s\n",
                                                               json->PrintUnformatted().c_str(), val->PrintUnformatted().c_str());
                                }
                            } else {
                                insert(view.access, view.primary_key, val, env, backtrace);
                                ++modified;
                            }
                        }
                    } catch (const query_language::broken_client_exc_t &e) {
                        ++errors;
                        if (reported_error == "") reported_error = e.message;
                    } catch (const query_language::runtime_exc_t &e) {
                        ++errors;
                        if (reported_error == "") reported_error = e.message + "\n" + e.backtrace.print();
                    }
                }
                std::string res_list = strprintf("\"modified\": %d, \"inserted\": %d, \"deleted\": %d, \"errors\": %d", modified, 0, deleted, errors);
                if (reported_error != "") {
                    res_list = strprintf("%s, \"first_error\": %s", res_list.c_str(),
                                         scoped_cJSON_t(cJSON_CreateString(reported_error.c_str())).Print().c_str());
                }
                res->add_response("{"+res_list+"}");
            }
            break;
        case WriteQuery::INSERT:
            {
                std::string pk = get_primary_key(w->mutable_insert()->mutable_table_ref(), env, backtrace);
                namespace_repo_t<rdb_protocol_t>::access_t ns_access =
                    eval(w->mutable_insert()->mutable_table_ref(), env, backtrace);

                int inserted = 0;
                if (w->insert().terms_size() == 1) {
                    Term *t = w->mutable_insert()->mutable_terms(0);
                    int32_t t_type = t->GetExtension(extension::inferred_type);
                    boost::shared_ptr<json_stream_t> stream;
                    if (t_type == TERM_TYPE_JSON) {
                        boost::shared_ptr<scoped_cJSON_t> data = eval(t, env, backtrace.with("term:0"));
                        if (data->type() == cJSON_Object) {
                            ++inserted;
                            insert(ns_access, pk, data, env, backtrace.with("term:0"));
                        } else if (data->type() == cJSON_Array) {
                            stream.reset(new in_memory_stream_t(json_array_iterator_t(data->get()), env));
                        } else {
                            throw runtime_exc_t(strprintf("Cannon insert non-object %s.\n", data->Print().c_str()), backtrace);
                        }
                    } else if (t_type == TERM_TYPE_STREAM || t_type == TERM_TYPE_VIEW) {
                        stream = eval_stream(w->mutable_insert()->mutable_terms(0), env, backtrace.with("stream"));
                    } else {
                        unreachable("bad term type");
                    }

                    if (stream) {
                        int i = 0;
                        while (boost::shared_ptr<scoped_cJSON_t> data = stream->next()) {
                            ++inserted;
                            insert(ns_access, pk, data, env, backtrace.with(strprintf("stream:%d", i++)));
                        }
                    }
                } else {
                    for (int i = 0; i < w->insert().terms_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> data =
                            eval(w->mutable_insert()->mutable_terms(i), env, backtrace.with(strprintf("term:%d", i)));
                        ++inserted;
                        insert(ns_access, pk, data, env, backtrace.with(strprintf("term:%d", i)));
                    }
                }

                res->add_response(strprintf("{\"inserted\": %d}", inserted));
            }
            break;
        case WriteQuery::INSERTSTREAM:
            {
                std::string pk = get_primary_key(w->mutable_insert_stream()->mutable_table_ref(), env, backtrace);
                boost::shared_ptr<json_stream_t> stream = eval_stream(w->mutable_insert_stream()->mutable_stream(), env, backtrace.with("stream"));

                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w->mutable_insert_stream()->mutable_table_ref(), env, backtrace);

                int inserted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    inserted++;
                    insert(ns_access, pk, json, env, backtrace);
                }

                res->add_response(strprintf("{\"inserted\": %d}", inserted));
            }
            break;
        case WriteQuery::FOREACH: {
                boost::shared_ptr<json_stream_t> stream =
                    eval_stream(w->mutable_for_each()->mutable_stream(), env, backtrace.with("stream"));

                // The cJSON object we'll eventually return (named `lhs` because it's merged with `rhs` below).
                scoped_cJSON_t lhs(cJSON_CreateObject());

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scopes.scope, w->for_each().var(), json);

                    for (int i = 0; i < w->for_each().queries_size(); ++i) {
                        // Run the write query and retrieve the result.
                        rassert(res->response_size() == 0);
                        scoped_cJSON_t rhs(0);
                        try {
                            execute(w->mutable_for_each()->mutable_queries(i), env, res, backtrace.with(strprintf("query:%d", i)));
                            rassert(res->response_size() == 1);
                            rhs.reset(cJSON_Parse(res->response(0).c_str()));
                        } catch (runtime_exc_t &e) {
                            rhs.reset(cJSON_CreateObject());
                            rhs.AddItemToObject("errors", cJSON_CreateNumber(1.0));
                            std::string err = strprintf("Term %d of the foreach threw an error: %s\n", i,
                                                        (e.message + "\n" + e.backtrace.print()).c_str());
                            rhs.AddItemToObject("first_error", cJSON_CreateString(err.c_str()));
                        }
                        res->clear_response();

                        // Merge the result of the write query into `lhs`
                        scoped_cJSON_t merged(cJSON_merge(lhs.get(), rhs.get()));
                        for (int f = 0; f < merged.GetArraySize(); ++f) {
                            cJSON *el = merged.GetArrayItem(f);
                            rassert(el);
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
            }
            break;
        case WriteQuery::POINTUPDATE: {
            namespace_repo_t<rdb_protocol_t>::access_t ns_access =
                eval(w->mutable_point_update()->mutable_table_ref(), env, backtrace);
            std::string pk = get_primary_key(w->mutable_point_update()->mutable_table_ref(), env, backtrace);
            boost::shared_ptr<scoped_cJSON_t> id = eval(w->mutable_point_update()->mutable_key(), env, backtrace);
            point_modify::result_t mres =
                point_modify(ns_access, pk, id->get(), point_modify::UPDATE, env, w->point_update().mapping(), backtrace);
            rassert(mres == point_modify::MODIFIED || mres == point_modify::SKIPPED);
            res->add_response(strprintf("{\"updated\": %d, \"skipped\": %d, \"errors\": %d}",
                                        mres == point_modify::MODIFIED, mres == point_modify::SKIPPED, 0));
            } break;
        case WriteQuery::POINTDELETE:
            {
                int deleted = -1;
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w->mutable_point_delete()->mutable_table_ref(), env, backtrace);
                boost::shared_ptr<scoped_cJSON_t> id = eval(w->mutable_point_delete()->mutable_key(), env, backtrace.with("key"));
                point_delete(ns_access, id, env, backtrace, &deleted);
                rassert(deleted == 0 || deleted == 1);

                res->add_response(strprintf("{\"deleted\": %d}", deleted));
            }
            break;
        case WriteQuery::POINTMUTATE: {
            namespace_repo_t<rdb_protocol_t>::access_t ns_access =
                eval(w->mutable_point_mutate()->mutable_table_ref(), env, backtrace);
            std::string pk = get_primary_key(w->mutable_point_mutate()->mutable_table_ref(), env, backtrace);
            boost::shared_ptr<scoped_cJSON_t> id = eval(w->mutable_point_mutate()->mutable_key(), env, backtrace);
            point_modify::result_t mres =
                point_modify(ns_access, pk, id->get(), point_modify::MUTATE, env, w->point_mutate().mapping(), backtrace);
            rassert(mres == point_modify::MODIFIED || mres == point_modify::INSERTED ||
                    mres == point_modify::DELETED  || mres == point_modify::NOP);
            res->add_response(strprintf("{\"modified\": %d, \"inserted\": %d, \"deleted\": %d, \"errors\": %d}",
                mres == point_modify::MODIFIED, mres == point_modify::INSERTED, mres == point_modify::DELETED, 0));
        } break;
        default:
            unreachable();
    }
}

//This doesn't create a scope for the evals
void eval_let_binds(Term::Let *let, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    // Go through the bindings in a let and add them one by one
    for (int i = 0; i < let->binds_size(); ++i) {
        backtrace_t backtrace_bind = backtrace.with(strprintf("bind:%s", let->binds(i).var().c_str()));
        term_info_t type = get_term_type(let->binds(i).term(), &env->scopes.type_env, backtrace_bind);

        if (type.type == TERM_TYPE_JSON) {
            env->scopes.scope.put_in_scope(let->binds(i).var(),
                    eval(let->mutable_binds(i)->mutable_term(), env, backtrace_bind));
        } else if (type.type == TERM_TYPE_STREAM || type.type == TERM_TYPE_VIEW) {
            throw runtime_exc_t("Cannot bind streams/views to variable names", backtrace);
        } else if (type.type == TERM_TYPE_ARBITRARY) {
            eval(let->mutable_binds(i)->mutable_term(), env, backtrace_bind);
            unreachable("This term has type `TERM_TYPE_ARBITRARY`, so "
                "evaluating it must throw  `runtime_exc_t`.");
        }

        env->scopes.type_env.scope.put_in_scope(let->binds(i).var(),
                                     type);
    }
}

boost::shared_ptr<scoped_cJSON_t> eval_and_check(Term *t, runtime_environment_t *env, const backtrace_t &backtrace, int type, const std::string &msg) {
    boost::shared_ptr<scoped_cJSON_t> res = eval(t, env, backtrace);
    if (res->type() != type) {
        throw runtime_exc_t(msg, backtrace);
    }
    return res;
}

boost::shared_ptr<scoped_cJSON_t> eval_and_check_either(Term *t, runtime_environment_t *env, const backtrace_t &backtrace, int type1, int type2, const std::string &msg) {
    boost::shared_ptr<scoped_cJSON_t> res = eval(t, env, backtrace);
    if (res->type() != type1 && res->type() != type2) {
        throw runtime_exc_t(msg, backtrace);
    }
    return res;
}


boost::shared_ptr<scoped_cJSON_t> eval(Term *t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (t->type()) {
        case Term::VAR:
            return env->scopes.scope.get(t->var());
            break;
        case Term::LET:
            {
                // Push the scope
                variable_val_scope_t::new_scope_t new_scope(&env->scopes.scope);
                variable_type_scope_t::new_scope_t new_type_scope(&env->scopes.type_env.scope);

                eval_let_binds(t->mutable_let(), env, backtrace);

                return eval(t->mutable_let()->mutable_expr(), env, backtrace.with("expr"));
            }
            break;
        case Term::CALL:
            return eval(t->mutable_call(), env, backtrace);
            break;
        case Term::IF:
            {
                boost::shared_ptr<scoped_cJSON_t> test = eval_and_check_either(t->mutable_if_()->mutable_test(), env, backtrace.with("test"), cJSON_True, cJSON_False, "The IF test must evaluate to a boolean.");

                boost::shared_ptr<scoped_cJSON_t> res;
                if (test->type() == cJSON_True) {
                    res = eval(t->mutable_if_()->mutable_true_branch(), env, backtrace.with("true"));
                } else {
                    res = eval(t->mutable_if_()->mutable_false_branch(), env, backtrace.with("false"));
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
                    res->AddItemToArray(eval(t->mutable_array(i), env, backtrace.with(strprintf("elem:%d", i)))->release());
                }
                return res;
            }
            break;
        case Term::OBJECT:
            {
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateObject()));
                for (int i = 0; i < t->object_size(); ++i) {
                    std::string item_name(t->object(i).var());
                    res->AddItemToObject(item_name.c_str(), eval(t->mutable_object(i)->mutable_term(), env, backtrace.with(strprintf("key:%s", item_name.c_str())))->release());
                }
                return res;
            }
            break;
        case Term::GETBYKEY:
            {
                std::string pk = get_primary_key(t->mutable_get_by_key()->mutable_table_ref(), env, backtrace);

                if (t->get_by_key().attrname() != pk) {
                    throw runtime_exc_t(strprintf("Attribute: %s is not the primary key (%s) and thus cannot be selected upon.", t->get_by_key().attrname().c_str(), pk.c_str()),
                        backtrace.with("attrname"));
                }

                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(t->mutable_get_by_key()->mutable_table_ref(), env, backtrace);

                boost::shared_ptr<scoped_cJSON_t> key = eval(t->mutable_get_by_key()->mutable_key(), env, backtrace.with("key"));

                try {
                    rdb_protocol_t::read_t read(rdb_protocol_t::point_read_t(store_key_t(key->Print())));
                    rdb_protocol_t::read_response_t res;
                    ns_access.get_namespace_if()->read(read, &res, order_token_t::ignore, env->interruptor);

                    rdb_protocol_t::point_read_response_t *p_res = boost::get<rdb_protocol_t::point_read_response_t>(&res.response);
                    return p_res->data;
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
            env->scopes.scope.dump(compiled ? NULL : &argnames, &argvals);

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
            if (env->scopes.implicit_attribute_value.has_value()) {
                object = env->scopes.implicit_attribute_value.get_value();
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

boost::shared_ptr<json_stream_t> eval_stream(Term *t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (t->type()) {
        case Term::LET:
            {
                // Push the scope
                variable_val_scope_t::new_scope_t new_scope(&env->scopes.scope);
                variable_type_scope_t::new_scope_t new_type_scope(&env->scopes.type_env.scope);

                eval_let_binds(t->mutable_let(), env, backtrace);

                return eval_stream(t->mutable_let()->mutable_expr(), env, backtrace.with("expr"));
            }
            break;
        case Term::CALL:
            return eval_stream(t->mutable_call(), env, backtrace);
            break;
        case Term::IF:
            {
                boost::shared_ptr<scoped_cJSON_t> test = eval_and_check_either(t->mutable_if_()->mutable_test(), env, backtrace.with("test"), cJSON_True, cJSON_False, "The IF test must evaluate to a boolean.");

                if (test->type() == cJSON_True) {
                    return eval_stream(t->mutable_if_()->mutable_true_branch(), env, backtrace.with("true"));
                } else {
                    return eval_stream(t->mutable_if_()->mutable_false_branch(), env, backtrace.with("false"));
                }
            }
            break;
        case Term::TABLE:
            {
                return eval_view(t->mutable_table(), env, backtrace).stream;
            }
            break;
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
            boost::shared_ptr<scoped_cJSON_t> arr = eval(t, env, backtrace);
            require_type(arr->get(), cJSON_Array, backtrace);
            json_array_iterator_t it(arr->get());
            return boost::shared_ptr<json_stream_t>(new in_memory_stream_t(it, env));
        } break;
        default:
            unreachable();
            break;
    }
    unreachable();
}

boost::shared_ptr<scoped_cJSON_t> eval(Term::Call *c, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (c->builtin().type()) {
        //JSON -> JSON
        case Builtin::NOT:
            {
                boost::shared_ptr<scoped_cJSON_t> data = eval(c->mutable_args(0), env, backtrace.with("arg:0"));

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
                    data = eval(c->mutable_args(0), env, backtrace.with("arg:0"));
                } else {
                    rassert(env->scopes.implicit_attribute_value.has_value());
                    data = env->scopes.implicit_attribute_value.get_value();
                }

                if (!data->type() == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object", backtrace.with("arg:0"));
                }

                cJSON *value = data->GetObjectItem(c->builtin().attr().c_str());

                if (!value) {
                    throw runtime_exc_t("Object is missing attribute \"" + c->builtin().attr() + "\"", backtrace.with("attr"));
                }

                return shared_scoped_json(cJSON_DeepCopy(value));
            }
            break;
        case Builtin::HASATTR:
        case Builtin::IMPLICIT_HASATTR:
            {
                boost::shared_ptr<scoped_cJSON_t> data;
                if (c->builtin().type() == Builtin::HASATTR) {
                    data = eval(c->mutable_args(0), env, backtrace.with("arg:0"));
                } else {
                    rassert(env->scopes.implicit_attribute_value.has_value());
                    data = env->scopes.implicit_attribute_value.get_value();
                }

                if (!data->type() == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object", backtrace.with("arg:0"));
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
                data = eval(c->mutable_args(0), env, backtrace.with("arg:0"));
            } else {
                rassert(c->builtin().type() == Builtin::IMPLICIT_WITHOUT);
                rassert(env->scopes.implicit_attribute_value.has_value());
                data = env->scopes.implicit_attribute_value.get_value();
            }
            if (!data->type() == cJSON_Object) {
                throw runtime_exc_t("Data must be an object", backtrace.with("arg:0"));
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
                    data = eval(c->mutable_args(0), env, backtrace.with("arg:0"));
                } else {
                    rassert(env->scopes.implicit_attribute_value.has_value());
                    data = env->scopes.implicit_attribute_value.get_value();
                }

                if (!data->type() == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object", backtrace.with("arg:0"));
                }

                boost::shared_ptr<scoped_cJSON_t> res = shared_scoped_json(cJSON_CreateObject());

                for (int i = 0; i < c->builtin().attrs_size(); ++i) {
                    cJSON *item = data->GetObjectItem(c->builtin().attrs(i).c_str());
                    if (!item) {
                        throw runtime_exc_t("Attempting to pick missing attribute.", backtrace.with(strprintf("attrs:%d", i)));
                    } else {
                        res->AddItemToObject(item->string, cJSON_DeepCopy(item));
                    }
                }
                return res;
            }
            break;
        case Builtin::MAPMERGE:
            {
                boost::shared_ptr<scoped_cJSON_t> left  = eval_and_check(c->mutable_args(0), env, backtrace.with("arg:0"),
                                                                         cJSON_Object, "Data must be an object"),
                                                  right = eval_and_check(c->mutable_args(1), env, backtrace.with("arg:1"),
                                                                         cJSON_Object, "Data must be an object");
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_merge(left->get(), right->get())));
            }
            break;
        case Builtin::ARRAYAPPEND:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array = eval_and_check(c->mutable_args(0), env, backtrace.with("arg:0"),
                    cJSON_Array, "The first argument must be an array.");
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(array->DeepCopy()));
                res->AddItemToArray(eval(c->mutable_args(1), env, backtrace.with("arg:1"))->release());
                return res;
            }
            break;
        case Builtin::SLICE:
            {
                int start, stop;

                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array = eval_and_check(c->mutable_args(0), env, backtrace.with("arg:0"),
                                                                         cJSON_Array, "The first argument must be an array.");

                int length = array->GetArraySize();

                // Check second arg type
                {
                    boost::shared_ptr<scoped_cJSON_t> start_json = eval_and_check_either(c->mutable_args(1), env, backtrace.with("arg:1"), cJSON_NULL, cJSON_Number, "Slice start must be null or an integer.");
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
                    boost::shared_ptr<scoped_cJSON_t> stop_json = eval_and_check_either(c->mutable_args(2), env, backtrace.with("arg:2"), cJSON_NULL, cJSON_Number, "Slice stop must be null or an integer.");
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
                std::vector<std::string> argnames;
                std::vector<boost::shared_ptr<scoped_cJSON_t> > values;
                env->scopes.scope.dump(&argnames, &values);

                if (c->args_size() == 0) {
                    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNull()));
                }

                boost::shared_ptr<scoped_cJSON_t> arg = eval(c->mutable_args(0), env, backtrace.with("arg:0"));

                if (arg->type() == cJSON_Number) {
                    double result = arg->get()->valuedouble;

                    for (int i = 1; i < c->args_size(); ++i) {
                        arg = eval_and_check(c->mutable_args(i), env, backtrace.with(strprintf("arg:%d", i)), cJSON_Number, "Cannot ADD numbers to non-numbers");
                        result += arg->get()->valuedouble;
                    }

                    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(safe_cJSON_CreateNumber(result, backtrace)));
                } else if (arg->type() == cJSON_Array) {
                    boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(arg->DeepCopy()));
                    for (int i = 1; i < c->args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg2 = eval_and_check(c->mutable_args(i), env, backtrace.with(strprintf("arg:%d", i)), cJSON_Array, "Cannot ADD arrays to non-arrays");
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
                    boost::shared_ptr<scoped_cJSON_t> arg = eval_and_check(c->mutable_args(0), env, backtrace.with("arg:0"), cJSON_Number, "All operands to SUBTRACT must be numbers.");
                    if (c->args_size() == 1) {
                        result = -arg->get()->valuedouble;  // (- x) is negate
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c->args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg2 = eval_and_check(c->mutable_args(i), env, backtrace.with(strprintf("arg:%d", i)), cJSON_Number, "All operands to SUBTRACT must be numbers.");
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
                    boost::shared_ptr<scoped_cJSON_t> arg = eval_and_check(c->mutable_args(i), env, backtrace.with(strprintf("arg:%d", i)), cJSON_Number, "All operands of MULTIPLY must be numbers.");
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
                    boost::shared_ptr<scoped_cJSON_t> arg = eval_and_check(c->mutable_args(0), env, backtrace.with("arg:0"), cJSON_Number, "All operands to DIVIDE must be numbers.");
                    if (c->args_size() == 1) {
                        result = 1.0 / arg->get()->valuedouble;  // (/ x) is reciprocal
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c->args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg2 = eval_and_check(c->mutable_args(i), env, backtrace.with(strprintf("arg:%d", i)), cJSON_Number, "All operands to DIVIDE must be numbers.");
                        result /= arg2->get()->valuedouble;
                    }
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(safe_cJSON_CreateNumber(result, backtrace)));
                return res;
            }
            break;
        case Builtin::MODULO:
            {
                boost::shared_ptr<scoped_cJSON_t> lhs = eval_and_check(c->mutable_args(0), env, backtrace.with("arg:0"), cJSON_Number, "First operand of MOD must be a number."),
                                                  rhs = eval_and_check(c->mutable_args(1), env, backtrace.with("arg:1"), cJSON_Number, "Second operand of MOD must be a number.");

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(safe_cJSON_CreateNumber(fmod(lhs->get()->valuedouble, rhs->get()->valuedouble), backtrace)));
                return res;
            }
            break;
        case Builtin::COMPARE:
            {
                bool result = true;

                boost::shared_ptr<scoped_cJSON_t> lhs = eval(c->mutable_args(0), env, backtrace.with("arg:0"));

                for (int i = 1; i < c->args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> rhs = eval(c->mutable_args(i), env, backtrace.with(strprintf("arg:%d", i)));

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
            boost::shared_ptr<json_stream_t> stream = eval_stream(c, env, backtrace);
            boost::shared_ptr<scoped_cJSON_t>
                arr(new scoped_cJSON_t(cJSON_CreateArray()));
            boost::shared_ptr<scoped_cJSON_t> json;
            while ((json = stream->next())) {
                arr->AddItemToArray(json->release());
            }
            return arr;
        } break;
        case Builtin::ARRAYTOSTREAM:
            unreachable("eval called on a function that returns a stream (use eval_stream instead).");
            break;
        case Builtin::LENGTH:
            {
                int length = 0;

                if (c->args(0).GetExtension(extension::inferred_type) == TERM_TYPE_JSON)
                {
                    // Check first arg type
                    boost::shared_ptr<scoped_cJSON_t> array = eval_and_check(c->mutable_args(0), env, backtrace.with("arg:0"), cJSON_Array, "LENGTH argument must be an array.");
                    length = array->GetArraySize();
                } else {
                    boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));
                    while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                        ++length;
                    }
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(safe_cJSON_CreateNumber(length, backtrace)));
            }
            break;
        case Builtin::NTH:
            if (c->args(0).GetExtension(extension::inferred_type) == TERM_TYPE_JSON) {
                boost::shared_ptr<scoped_cJSON_t> array = eval_and_check(c->mutable_args(0), env, backtrace.with("arg:0"),
                    cJSON_Array, "The first argument must be an array.");

                boost::shared_ptr<scoped_cJSON_t> index_json = eval_and_check(c->mutable_args(1), env, backtrace.with("arg:1"),
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
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> index_json = eval_and_check(c->mutable_args(1), env, backtrace.with("arg:1"), cJSON_Number, "The second argument must be an integer.");
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
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    res->AddItemToArray(json->DeepCopy());
                }

                return res;
            }
            break;
        case Builtin::REDUCE:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));

                try {
                    return boost::get<boost::shared_ptr<scoped_cJSON_t> >(stream->apply_terminal(c->builtin().reduce()));
                } catch (const boost::bad_get &) {
                    crash("Expected a json atom... something is implemented wrong in the clustering code\n");
                }
            }
            break;
        case Builtin::GROUPEDMAPREDUCE: {
                Builtin::GroupedMapReduce *g = c->mutable_builtin()->mutable_grouped_map_reduce();
                shared_scoped_less_t comparator(backtrace);
                std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less_t> groups(comparator);
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    boost::shared_ptr<scoped_cJSON_t> group_mapped_row, value_mapped_row, reduced_row;

                    // Figure out which group we belong to
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scopes.scope, g->group_mapping().arg(), json);
                        implicit_value_setter_t impliciter(&env->scopes.implicit_attribute_value, json);
                        group_mapped_row = eval(g->mutable_group_mapping()->mutable_body(), env, backtrace.with("group_mapping"));
                    }

                    // Map the value for comfy reduction goodness
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scopes.scope, g->value_mapping().arg(), json);
                        implicit_value_setter_t impliciter(&env->scopes.implicit_attribute_value, json);
                        value_mapped_row = eval(g->mutable_value_mapping()->mutable_body(), env, backtrace.with("value_mapping"));
                    }

                    // Do the reduction
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scopes.scope);
                        std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less_t>::iterator
                            elem = groups.find(group_mapped_row);
                        if(elem != groups.end()) {
                            env->scopes.scope.put_in_scope(g->reduction().var1(),
                                                    (*elem).second);
                        } else {
                            env->scopes.scope.put_in_scope(g->reduction().var1(),
                                                    eval(g->mutable_reduction()->mutable_base(), env, backtrace.with("reduction").with("base")));
                        }
                        env->scopes.scope.put_in_scope(g->reduction().var2(), value_mapped_row);

                        reduced_row = eval(g->mutable_reduction()->mutable_body(), env, backtrace.with("reduction").with("body"));
                    }

                    // Phew, update the relevant group
                    groups[group_mapped_row] = reduced_row;

                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));
                std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less_t>::iterator it;
                for (it = groups.begin(); it != groups.end(); ++it) {
                    scoped_cJSON_t obj(cJSON_CreateObject());
                    obj.AddItemToObject("group", it->first->release());
                    obj.AddItemToObject("reduction", it->second->release());
                    res->AddItemToArray(obj.release());
                }
                return res;
        } break;
        case Builtin::ALL:
            {
                bool result = true;

                for (int i = 0; i < c->args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c->mutable_args(i), env, backtrace.with(strprintf("arg:%d", i)));
                    if (arg->type() != cJSON_False && arg->type() != cJSON_True) {
                        throw runtime_exc_t("All operands to ALL must be booleans.", backtrace.with(strprintf("arg:%d", i)));
                    }
                    if (arg->type() != cJSON_True) {
                        result = false;
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
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c->mutable_args(i), env, backtrace.with(strprintf("arg:%d", i)));
                    if (arg->type() != cJSON_False && arg->type() != cJSON_True) {
                        throw runtime_exc_t("All operands to ANY must be booleans.", backtrace.with(strprintf("arg:%d", i)));
                    }
                    if (arg->type() == cJSON_True) {
                        result = true;
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

predicate_t::predicate_t(const Predicate &_pred, runtime_environment_t _env, const backtrace_t &_backtrace)
    : pred(_pred), env(_env), backtrace(_backtrace)
{ }

bool predicate_t::operator()(boost::shared_ptr<scoped_cJSON_t> json) {
    variable_val_scope_t::new_scope_t scope_maker(&env.scopes.scope, pred.arg(), json);
    implicit_value_setter_t impliciter(&env.scopes.implicit_attribute_value, json);
    boost::shared_ptr<scoped_cJSON_t> a_bool = eval(pred.mutable_body(), &env, backtrace.with("predicate"));

    if (a_bool->type() == cJSON_True) {
        return true;
    } else if (a_bool->type() == cJSON_False) {
        return false;
    } else {
        throw runtime_exc_t("Predicate failed to evaluate to a bool", backtrace.with("predicate"));
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
                throw runtime_exc_t("OrderBy encountered a row missing attr " + cur.attr(), backtrace);
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

boost::shared_ptr<scoped_cJSON_t> map(std::string arg, Term *term, runtime_environment_t env, boost::shared_ptr<scoped_cJSON_t> val, const backtrace_t &backtrace) {
    variable_val_scope_t::new_scope_t scope_maker(&env.scopes.scope, arg, val);
    implicit_value_setter_t impliciter(&env.scopes.implicit_attribute_value, val);
    return eval(term, &env, backtrace);
}

boost::shared_ptr<scoped_cJSON_t> eval_mapping(Mapping m, const runtime_environment_t &env,
                                               boost::shared_ptr<scoped_cJSON_t> val, const backtrace_t &backtrace) {
    return map(m.arg(), m.mutable_body(), env, val, backtrace);
}

boost::shared_ptr<json_stream_t> concatmap(std::string arg, Term *term, runtime_environment_t env, boost::shared_ptr<scoped_cJSON_t> val, const backtrace_t &backtrace) {
    variable_val_scope_t::new_scope_t scope_maker(&env.scopes.scope, arg, val);
    implicit_value_setter_t impliciter(&env.scopes.implicit_attribute_value, val);
    return eval_stream(term, &env, backtrace);
}

boost::shared_ptr<json_stream_t> eval_stream(Term::Call *c, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
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
            boost::shared_ptr<scoped_cJSON_t> arr = eval(c, env, backtrace);
            require_type(arr->get(), cJSON_Array, backtrace);
            json_array_iterator_t it(arr->get());
            return boost::shared_ptr<json_stream_t>(new in_memory_stream_t(it, env));
        } break;
        case Builtin::FILTER:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));
                stream->add_transformation(c->builtin().filter());
                return stream;
            }
            break;
        case Builtin::MAP:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));
                stream->add_transformation(c->builtin().map());
                return stream;
            }
            break;
        case Builtin::CONCATMAP:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));
                stream->add_transformation(c->builtin().concat_map());
                return stream;
            }
            break;
        case Builtin::ORDERBY:
            {
                ordering_t o(c->builtin().order_by(), backtrace.with("order_by"));
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));

                boost::shared_ptr<in_memory_stream_t> sorted_stream(new in_memory_stream_t(stream, env));
                sorted_stream->sort(o);
                return sorted_stream;
            }
            break;
        case Builtin::DISTINCT:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));

                shared_scoped_less_t comparator(backtrace);

                return boost::shared_ptr<json_stream_t>(new distinct_stream_t<shared_scoped_less_t>(stream, comparator));
            }
            break;
        case Builtin::SLICE:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));

                int start, stop;
                bool stop_unbounded = false;

                {
                    boost::shared_ptr<scoped_cJSON_t> start_json = eval_and_check_either(c->mutable_args(1), env, backtrace.with("arg:1"), cJSON_NULL, cJSON_Number, "Slice start must be null or a nonnegative integer.");
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
                    boost::shared_ptr<scoped_cJSON_t> stop_json = eval_and_check_either(c->mutable_args(2), env, backtrace.with("arg:2"), cJSON_NULL, cJSON_Number, "Slice stop must be null or a nonnegative integer.");
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

                streams.push_back(eval_stream(c->mutable_args(0), env, backtrace.with("arg:0")));

                streams.push_back(eval_stream(c->mutable_args(1), env, backtrace.with("arg:1")));

                return boost::shared_ptr<json_stream_t>(new union_stream_t(streams));
            }
            break;
        case Builtin::ARRAYTOSTREAM:
            {
                boost::shared_ptr<scoped_cJSON_t> array = eval(c->mutable_args(0), env, backtrace.with("arg:0"));
                require_type(array->get(), cJSON_Array, backtrace);
                json_array_iterator_t it(array->get());

                return boost::shared_ptr<json_stream_t>(new in_memory_stream_t(it, env));
            }
            break;
        case Builtin::RANGE:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));

                boost::shared_ptr<scoped_cJSON_t> lowerbound, upperbound;

                Builtin::Range *r = c->mutable_builtin()->mutable_range();

                key_range_t range;

                if (r->has_lowerbound()) {
                    lowerbound = eval(r->mutable_lowerbound(), env, backtrace.with("lowerbound"));
                    if (lowerbound->type() != cJSON_Number && lowerbound->type() != cJSON_String) {
                        throw runtime_exc_t(strprintf("Lower bound of RANGE must be a string or a number, not %s.",
                                                      lowerbound->Print().c_str()), backtrace);
                    }
                }

                if (r->has_upperbound()) {
                    upperbound = eval(r->mutable_upperbound(), env, backtrace.with("upperbound"));
                    if (upperbound->type() != cJSON_Number && upperbound->type() != cJSON_String) {
                        throw runtime_exc_t(strprintf("Lower bound of RANGE must be a string or a number, not %s.",
                                                      upperbound->Print().c_str()), backtrace);
                    }
                }

                if (lowerbound && upperbound) {
                    range = key_range_t(key_range_t::closed, store_key_t(lowerbound->PrintLexicographic()),
                                        key_range_t::closed, store_key_t(upperbound->PrintLexicographic()));
                } else if (lowerbound) {
                    range = key_range_t(key_range_t::closed, store_key_t(lowerbound->PrintLexicographic()),
                                        key_range_t::none, store_key_t());
                } else if (upperbound) {
                    range = key_range_t(key_range_t::none, store_key_t(),
                                        key_range_t::closed, store_key_t(upperbound->PrintLexicographic()));
                }

                return boost::shared_ptr<json_stream_t>(
                    new range_stream_t(stream, range, r->attrname(), backtrace));
            }
            throw runtime_exc_t("Unimplemented: Builtin::RANGE", backtrace);
            break;
        default:
            crash("unreachable");
            break;
    }
    crash("unreachable");

}

namespace_repo_t<rdb_protocol_t>::access_t eval(
    TableRef *t, runtime_environment_t *env, const backtrace_t &bt)
    THROWS_ONLY(runtime_exc_t) {
    std::string table_name = t->table_name();
    std::string db_name = t->db_name();

    namespaces_semilattice_metadata_t<rdb_protocol_t> namespaces_metadata = env->namespaces_semilattice_metadata->get();
    databases_semilattice_metadata_t databases_metadata = env->databases_semilattice_metadata->get();

    metadata_searcher_t<namespace_semilattice_metadata_t<rdb_protocol_t> >
        ns_searcher(&namespaces_metadata.namespaces);
    metadata_searcher_t<database_semilattice_metadata_t>
        db_searcher(&databases_metadata.databases);

    uuid_t db_id = meta_get_uuid(db_searcher, db_name, "EVAL_DB "+db_name, bt);
    namespace_predicate_t pred(table_name, db_id);
    uuid_t id = meta_get_uuid(ns_searcher, pred, "EVAL_TABLE "+table_name, bt);

    return namespace_repo_t<rdb_protocol_t>::access_t(env->ns_repo,id,env->interruptor);
}

view_t eval_view(Term::Call *c, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
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
            unreachable("eval_view called on a function that does not return a view");
            break;
        case Builtin::FILTER:
            {
                view_t view = eval_view(c->mutable_args(0), env, backtrace.with("arg:0"));
                view.stream->add_transformation(c->builtin().filter());
                return view_t(view.access, view.primary_key, view.stream);
            }
            break;
        case Builtin::ORDERBY:
            {
                ordering_t o(c->builtin().order_by(), backtrace.with("order_by"));
                view_t view = eval_view(c->mutable_args(0), env, backtrace.with("arg:0"));

                boost::shared_ptr<in_memory_stream_t> sorted_stream(new in_memory_stream_t(view.stream, env));
                sorted_stream->sort(o);
                return view_t(view.access, view.primary_key, sorted_stream);
            }
            break;
        case Builtin::SLICE:
            {
                view_t view = eval_view(c->mutable_args(0), env, backtrace.with("arg:0"));

                int start, stop;
                bool stop_unbounded = false;

                {
                    boost::shared_ptr<scoped_cJSON_t> start_json = eval_and_check_either(c->mutable_args(1), env, backtrace.with("arg:1"), cJSON_NULL, cJSON_Number, "Slice start must be null or a nonnegative integer.");
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
                    boost::shared_ptr<scoped_cJSON_t> stop_json = eval_and_check_either(c->mutable_args(2), env, backtrace.with("arg:2"), cJSON_NULL, cJSON_Number, "Slice stop must be null or a nonnegative integer.");
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

view_t eval_view(Term *t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (t->type()) {
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
            crash("eval_view called on incompatible Term");
            break;
        case Term::CALL:
            return eval_view(t->mutable_call(), env, backtrace);
            break;
        case Term::IF:
            {
                boost::shared_ptr<scoped_cJSON_t> test = eval_and_check_either(t->mutable_if_()->mutable_test(), env, backtrace.with("test"), cJSON_True, cJSON_False, "The IF test must evaluate to a boolean.");

                if (test->type() == cJSON_True) {
                    return eval_view(t->mutable_if_()->mutable_true_branch(), env, backtrace.with("true"));
                } else {
                    return eval_view(t->mutable_if_()->mutable_false_branch(), env, backtrace.with("false"));
                }
            }
            break;
        case Term::GETBYKEY:
            crash("unimplemented");
            break;
        case Term::TABLE:
            return eval_view(t->mutable_table(), env, backtrace.with("table_ref"));
            break;
        default:
            unreachable();
    }
    unreachable();
}

view_t eval_view(Term::Table *t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(t->mutable_table_ref(), env, backtrace);
    std::string pk = get_primary_key(t->mutable_table_ref(), env, backtrace);
    boost::shared_ptr<json_stream_t> stream(new batched_rget_stream_t(ns_access, env->interruptor, key_range_t::universe(), 100, backtrace, env->scopes));
    return view_t(ns_access, pk, stream);
}

} //namespace query_language
