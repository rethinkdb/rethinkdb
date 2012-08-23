#include "rdb_protocol/query_language.hpp"

#include <math.h>

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "clustering/administration/main/ports.hpp"
#include "clustering/administration/suggester.hpp"
#include "http/json.hpp"
#include "rdb_protocol/internal_extensions.pb.h"
#include "rdb_protocol/js.hpp"

namespace query_language {


/* Convenience function for making the horrible easy. */
boost::shared_ptr<scoped_cJSON_t> shared_scoped_json(cJSON *json) {
    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(json));
}


void check_protobuf(bool cond) {
    if (!cond) {
        BREAKPOINT;
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
                        backtrace.with(strprintf("bind:%s", t.let().binds(i).var().c_str()))
                    ));
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
            check_function_args(c, TERM_TYPE_JSON, 1, env, &deterministic, backtrace);
            return term_info_t(TERM_TYPE_JSON, deterministic);
            break;
        case Builtin::IMPLICIT_GETATTR:
        case Builtin::IMPLICIT_HASATTR:
        case Builtin::IMPLICIT_PICKATTRS:
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
                check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);

                implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                check_mapping_type(b.map().mapping(), TERM_TYPE_JSON, env, &deterministic, deterministic, backtrace.with("mapping"));
                return term_info_t(TERM_TYPE_STREAM, deterministic);
            }
            break;
        case Builtin::CONCATMAP:
            {
                check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);

                implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                check_mapping_type(b.concat_map().mapping(), TERM_TYPE_STREAM, env, &deterministic, deterministic, backtrace.with("mapping"));
                return term_info_t(TERM_TYPE_STREAM, deterministic);
            }
            break;
        case Builtin::DISTINCT:
            {
                check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);

                return term_info_t(TERM_TYPE_STREAM, deterministic);
            }
            break;
        case Builtin::FILTER:
            {
                check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);
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
                check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);
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
                check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);
                implicit_value_t<term_info_t>::impliciter_t impliciter(&env->implicit_type, term_info_t(TERM_TYPE_JSON, deterministic)); //make the implicit value be of type json
                check_reduction_type(b.reduce(), env, &deterministic, deterministic, backtrace.with("reduce"));
                return term_info_t(TERM_TYPE_JSON, false); //This is always false because we can't be sure the functions is associative or commutative
            }
            break;
        case Builtin::GROUPEDMAPREDUCE:
            {
                check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);
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
            check_function_args(c, TERM_TYPE_STREAM, 1, env, &deterministic, backtrace);
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

    switch (w.type()) {
        case WriteQuery::UPDATE: {
            check_protobuf(w.has_update());
            check_term_type(w.update().view(), TERM_TYPE_VIEW, env, is_det_out, backtrace.with("view"));
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
            check_mapping_type(w.mutate().mapping(), TERM_TYPE_JSON, env, is_det_out, *is_det_out, backtrace.with("mapping"));
            break;
        }
        case WriteQuery::INSERT: {
            check_protobuf(w.has_insert());
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
                check_term_type(w.for_each().stream(), TERM_TYPE_STREAM, env, is_det_out, backtrace.with("stream"));

                new_scope_t scope_maker(&env->scope, w.for_each().var(), term_info_t(TERM_TYPE_JSON, *is_det_out));
                for (int i = 0; i < w.for_each().queries_size(); ++i) {
                    check_write_query_type(w.for_each().queries(i), env, is_det_out, backtrace.with(strprintf("query:%d", i)));
                }
            }
            break;
        case WriteQuery::POINTUPDATE: {
            check_protobuf(w.has_point_update());
            check_term_type(w.point_update().key(), TERM_TYPE_JSON, env, is_det_out, backtrace.with("key"));
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
            check_mapping_type(w.point_mutate().mapping(), TERM_TYPE_JSON, env, is_det_out, *is_det_out, backtrace.with("mapping"));
            break;
        }
        default:
            unreachable("unhandled WriteQuery");
    }
}

void check_tableop_query_type(const TableopQuery &t) {
    check_protobuf(TableopQuery::TableopQueryType_IsValid(t.type()));
    switch(t.type()) {
    case TableopQuery::CREATE:
        check_protobuf(t.has_create());
        check_protobuf(!t.has_drop());
        break;
    case TableopQuery::DROP:
        check_protobuf(!t.has_create());
        check_protobuf(t.has_drop());
        break;
    case TableopQuery::LIST:
        check_protobuf(!t.has_create());
        check_protobuf(!t.has_drop());
        break;
    default: unreachable("Unhandled TableopQuery.");
    }
}

void check_query_type(const Query &q, type_checking_environment_t *env, bool *is_det_out, const backtrace_t &backtrace) {
    check_protobuf(Query::QueryType_IsValid(q.type()));
    switch (q.type()) {
    case Query::READ:
        check_protobuf(q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_protobuf(!q.has_tableop_query());
        check_read_query_type(q.read_query(), env, is_det_out, backtrace);
        break;
    case Query::WRITE:
        check_protobuf(q.has_write_query());
        check_protobuf(!q.has_read_query());
        check_protobuf(!q.has_tableop_query());
        check_write_query_type(q.write_query(), env, is_det_out, backtrace);
        break;
    case Query::CONTINUE:
        check_protobuf(!q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_protobuf(!q.has_tableop_query());
        break;
    case Query::STOP:
        check_protobuf(!q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_protobuf(!q.has_tableop_query());
        break;
    case Query::TABLEOP:
        check_protobuf(q.has_tableop_query());
        check_protobuf(!q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_tableop_query_type(q.tableop_query());
        break;
    default:
        unreachable("unhandled Query");
    }
}

std::string get_primary_key(const std::string &table_name, runtime_environment_t *env, const backtrace_t &backtrace) {
    const char *status;
    boost::optional< std::pair<namespace_id_t, deletable_t< namespace_semilattice_metadata_t<rdb_protocol_t> > > > ns_info =
        metadata_get_by_name(env->semilattice_metadata->get().rdb_namespaces.namespaces,
                             table_name, &status);

    if (!ns_info) {
        rassert(status);
        throw runtime_exc_t(strprintf("Namespace %s not found with error: %s",
                                      table_name.c_str(), status), backtrace);
    }

    if (ns_info->second.get().primary_key.in_conflict()) {
        throw runtime_exc_t(strprintf("Namespace %s has an in conflict primary key, this should really never happen.", ns_info->second.get().name.get().c_str()), backtrace);
    }

    return ns_info->second.get().primary_key.get();
}


boost::shared_ptr<js::runner_t> runtime_environment_t::get_js_runner() {
    pool->assert_thread();
    if (!js_runner->connected()) {
        js_runner->begin(pool);
    }
    return js_runner;
}

void parse_tableop_create(const TableopQuery::Create &c, std::string *datacenter,
                          std::string *db_name, std::string *table_name,
                          std::string *primary_key) {
    *datacenter = c.datacenter();
    if (c.table_ref().has_db_name()) {
        *db_name = c.table_ref().db_name();
    } else {
        *db_name = "Welcome-db";
    }
    *table_name = c.table_ref().table_name();
    if (c.has_primary_key()) {
        *primary_key = c.primary_key();
    } else {
        *primary_key = "id";
    }
}

void execute_tableop(TableopQuery *t, runtime_environment_t *env, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t, broken_client_exc_t) {
    cluster_semilattice_metadata_t metadata = env->semilattice_metadata->get();
    switch(t->type()) {
    case TableopQuery::CREATE: {
        const char *status;
        std::string dc_name, db_name, table_name, primary_key;
        parse_tableop_create(t->create(), &dc_name, &db_name, &table_name, &primary_key);

        //Make sure namespace doesn't exist before we go any further.
        metadata_get_by_name(metadata.rdb_namespaces.namespaces, table_name, &status);
        if (status != METADATA_ERR_NONE) {
            if (status == METADATA_SUCCESS) {
                throw runtime_exc_t(strprintf("Table %s already exists!",
                                              table_name.c_str()), backtrace);
            } else {
                throw runtime_exc_t(strprintf("Table %s creation failed with error: %s",
                                              table_name.c_str(), status), backtrace);
            }
        }

        // Get datacenter ID.
        boost::optional<uuid_t> opt_datacenter_id = metadata_get_uuid_by_name(
            metadata.datacenters.datacenters, dc_name, &status);
        if (!opt_datacenter_id) {
            rassert(status);
            throw runtime_exc_t(strprintf("No datacenter %s found with error: %s",
                                          dc_name.c_str(), status), backtrace);
        }
        uuid_t dc_id = *opt_datacenter_id;

        // Get database ID.
        boost::optional<uuid_t> opt_database_id = metadata_get_uuid_by_name(
            metadata.databases.databases, db_name, &status);
        if (!opt_database_id) {
            rassert(status);
            throw runtime_exc_t(strprintf("No database %s found with error: %s",
                                          db_name.c_str(), status), backtrace);
        }
        uuid_t db_id = *opt_database_id;

        // Create namespace, insert into metadata, then join into real metadata.
        uuid_t namespace_id = generate_uuid();
        //TODO(mlucy): port number?
        namespace_semilattice_metadata_t<rdb_protocol_t> ns =
            new_namespace<rdb_protocol_t>(
                env->this_machine, db_id, dc_id, table_name, primary_key,
                port_constants::namespace_port);
        metadata.rdb_namespaces.namespaces.insert(std::make_pair(namespace_id, ns));
        fill_in_blueprints(&metadata, env->directory_metadata->get(), env->this_machine);
        env->semilattice_metadata->join(metadata);

        /* The following is an ugly hack, but it's probably what we want.  It
           takes about a third of a second for the new namespace to get to the
           point where we can do reads/writes on it.  We don't want to return
           until that has happened, so we try to do a read every `poll_ms`
           milliseconds until one succeeds, then return. */

        int64_t poll_ms = 200; //with this value, usually polls twice
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
            throw runtime_exc_t("Query interrupted, probably by user.", backtrace);
        }
        res->set_status_code(Response::SUCCESS_EMPTY);
    } break;
    case TableopQuery::DROP: {
        const char *status;
        std::string table_name = t->drop().table_name();

        // Get namespace ID.
        boost::optional<std::pair<uuid_t, deletable_t< namespace_semilattice_metadata_t<rdb_protocol_t> > > > ns_metadata =
            metadata_get_by_name(metadata.rdb_namespaces.namespaces, table_name, &status);
        if (!ns_metadata) {
            rassert(status);
            throw runtime_exc_t(strprintf("No table %s found with error: %s",
                                          table_name.c_str(), status), backtrace);
        }
        rassert(!ns_metadata->second.is_deleted());

        // Delete namespace
        //TODO: make metadata_get_by_name return an iterator instead to skip this step?
        metadata.rdb_namespaces.namespaces.find(ns_metadata->first)
            ->second.mark_deleted();
        env->semilattice_metadata->join(metadata);
        res->set_status_code(Response::SUCCESS_EMPTY); //return immediately
    } break;
    case TableopQuery::LIST: {
        for (namespaces_semilattice_metadata_t<rdb_protocol_t>::namespace_map_t::
                 iterator it = metadata.rdb_namespaces.namespaces.begin();
             it != metadata.rdb_namespaces.namespaces.end();
             ++it) {
            if (it->second.is_deleted()) continue;
            namespace_semilattice_metadata_t<rdb_protocol_t> ns_metadata =
                it->second.get();
            scoped_cJSON_t this_namespace(cJSON_CreateObject());
            bool conflicted = false;

            this_namespace.AddItemToObject(
                "uuid", cJSON_CreateString(uuid_to_str(it->first).c_str()));

            if (ns_metadata.name.in_conflict()) {
                this_namespace.AddItemToObject("table_name", cJSON_CreateNull());
                conflicted = true;
            } else {
                this_namespace.AddItemToObject(
                    "table_name", cJSON_CreateString(ns_metadata.name.get().c_str()));
            }

            if (ns_metadata.database.in_conflict()) {
                this_namespace.AddItemToObject("db_name", cJSON_CreateNull());
                conflicted = true;
            } else {
                uuid_t table_id = ns_metadata.database.get();
                databases_semilattice_metadata_t::database_map_t::iterator
                    db_metadata_it = metadata.databases.databases.find(table_id);
                if (db_metadata_it == metadata.databases.databases.end()
                    || db_metadata_it->second.is_deleted()) {
                    logERR("Namespace metadata contains invalid database ID!");
                    throw runtime_exc_t("Namespace metadata is corrupted!", backtrace);
                }
                database_semilattice_metadata_t db_metadata =
                    db_metadata_it->second.get();
                if (db_metadata.name.in_conflict()) {
                    this_namespace.AddItemToObject("db_name", cJSON_CreateNull());
                    conflicted = true;
                } else {
                    this_namespace.AddItemToObject(
                        "db_name", cJSON_CreateString(db_metadata.name.get().c_str()));

                }
            }

            this_namespace.AddItemToObject("conflicted", cJSON_CreateBool(conflicted));
            res->add_response(this_namespace.PrintUnformatted());
        }
        res->set_status_code(Response::SUCCESS_STREAM);
    } break;
    default: crash("unreachable");
    }
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
    case Query::TABLEOP:
        execute_tableop(q->mutable_tableop_query(), env, res, backtrace);
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

    try {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t(cJSON_print_std_string(data->GetObjectItem(pk.c_str()))), data));
        rdb_protocol_t::write_response_t response;
        ns_access.get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, cJSON *id, runtime_environment_t *env, const backtrace_t &backtrace) {
    try {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_delete_t(store_key_t(cJSON_print_std_string(id))));
        rdb_protocol_t::write_response_t response;
        ns_access.get_namespace_if()->write(write, &response, order_token_t::ignore, env->interruptor);
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> id, runtime_environment_t *env, const backtrace_t &backtrace) {
    point_delete(ns_access, id->get(), env, backtrace);
}

void execute(WriteQuery *w, runtime_environment_t *env, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t, broken_client_exc_t) {
    //TODO: When writes can return different responses, be more clever.
    res->set_status_code(Response::SUCCESS_JSON);
    switch (w->type()) {
        case WriteQuery::UPDATE:
            {
                view_t view = eval_view(w->mutable_update()->mutable_view(), env, backtrace.with("view"));

                int updated = 0,
                    error = 0,
                    skipped = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope, w->update().mapping().arg(), json);
                    boost::shared_ptr<scoped_cJSON_t> val = eval(w->mutable_update()->mutable_mapping()->mutable_body(), env, backtrace.with("mapping"));
                    if (val->type() == cJSON_NULL) {
                        skipped++;
                    } else if (!cJSON_Equal(json->GetObjectItem(view.primary_key.c_str()),
                                            val->GetObjectItem(view.primary_key.c_str()))) {
                        error++;
                    } else {
                        insert(view.access, view.primary_key, val, env, backtrace);
                        updated++;
                    }
                }

                res->add_response(strprintf("{\"updated\": %d, \"skipped\": %d, \"errors\": %d}", updated, skipped, error));
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

                int modified = 0, deleted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope, w->mutate().mapping().arg(), json);
                    boost::shared_ptr<scoped_cJSON_t> val = eval(w->mutable_mutate()->mutable_mapping()->mutable_body(), env, backtrace.with("mapping"));

                    if (val->type() == cJSON_NULL) {
                        point_delete(view.access, json->GetObjectItem(view.primary_key.c_str()), env, backtrace);
                        ++deleted;
                    } else {
                        insert(view.access, view.primary_key, val, env, backtrace);
                        ++modified;
                    }
                }

                res->add_response(strprintf("{\"modified\": %d, \"deleted\": %d}", modified, deleted));
            }
            break;
        case WriteQuery::INSERT:
            {
                std::string pk = get_primary_key(w->mutable_insert()->mutable_table_ref()->table_name(), env, backtrace);
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w->mutable_insert()->mutable_table_ref(), env, backtrace);

                for (int i = 0; i < w->insert().terms_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> data = eval(w->mutable_insert()->mutable_terms(i), env,
                        backtrace.with(strprintf("term:%d", i)));

                    insert(ns_access, pk, data, env, backtrace.with(strprintf("term:%d", i)));
                }

                res->add_response(strprintf("{\"inserted\": %d}", w->insert().terms_size()));
            }
            break;
        case WriteQuery::INSERTSTREAM:
            {
                std::string pk = get_primary_key(w->mutable_insert_stream()->mutable_table_ref()->table_name(), env, backtrace);
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
        case WriteQuery::FOREACH:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(w->mutable_for_each()->mutable_stream(), env, backtrace.with("stream"));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope, w->for_each().var(), json);

                    for (int i = 0; i < w->for_each().queries_size(); ++i) {
                        execute(w->mutable_for_each()->mutable_queries(i), env, res, backtrace.with(strprintf("query:%d", i)));
                    }
                }
            }
            break;
        case WriteQuery::POINTUPDATE:
            {
                //First we need to grab the value the easiest way to do this is to just construct a term and evaluate it.
                Term get;
                get.set_type(Term::GETBYKEY);
                Term::GetByKey get_by_key;
                *get_by_key.mutable_table_ref() = w->point_update().table_ref();
                get_by_key.set_attrname(w->point_update().attrname());
                *get_by_key.mutable_key() = w->point_update().key();
                *get.mutable_get_by_key() = get_by_key;

                boost::shared_ptr<scoped_cJSON_t> original_val = eval(&get, env, backtrace);
                new_val_scope_t scope_maker(&env->scope, w->point_update().mapping().arg(), original_val);

                boost::shared_ptr<scoped_cJSON_t> new_val = eval(w->mutable_point_update()->mutable_mapping()->mutable_body(), env, backtrace.with("mapping"));

                /* Now we insert the new value. */
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w->mutable_point_update()->mutable_table_ref(), env, backtrace);

                /* Get the primary key */
                std::string pk = get_primary_key(w->mutable_point_update()->mutable_table_ref()->table_name(), env, backtrace);

                /* Make sure that the primary key wasn't changed. */
                if (!cJSON_Equal(cJSON_GetObjectItem(original_val->get(), pk.c_str()),
                                 cJSON_GetObjectItem(new_val->get(), pk.c_str()))) {
                    throw runtime_exc_t(strprintf("Point updates are not allowed to change the primary key (%s).", pk.c_str()), backtrace);
                }


                insert(ns_access, pk, new_val, env, backtrace);

                res->add_response(strprintf("{\"updated\": %d, \"errors\": %d}", 1, 0));
            }
            break;
        case WriteQuery::POINTDELETE:
            {
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w->mutable_point_delete()->mutable_table_ref(), env, backtrace);
                boost::shared_ptr<scoped_cJSON_t> id = eval(w->mutable_point_delete()->mutable_key(), env, backtrace.with("key"));
                point_delete(ns_access, id, env, backtrace);

                res->add_response(strprintf("{\"deleted\": %d}", 1));
            }
            break;
        case WriteQuery::POINTMUTATE:
            throw runtime_exc_t("Unimplemented: POINTMUTATE", backtrace);
            break;
        default:
            unreachable();
    }
}

//This doesn't create a scope for the evals
void eval_let_binds(Term::Let *let, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    // Go through the bindings in a let and add them one by one
    for (int i = 0; i < let->binds_size(); ++i) {
        backtrace_t backtrace_bind = backtrace.with(strprintf("bind:%s", let->binds(i).var().c_str()));
        term_info_t type = get_term_type(let->binds(i).term(), &env->type_env, backtrace_bind);

        if (type.type == TERM_TYPE_JSON) {
            env->scope.put_in_scope(let->binds(i).var(),
                    eval(let->mutable_binds(i)->mutable_term(), env, backtrace_bind));
        } else if (type.type == TERM_TYPE_STREAM || type.type == TERM_TYPE_VIEW) {
            throw runtime_exc_t("Cannot bind streams/views to variable names", backtrace);
        } else if (type.type == TERM_TYPE_ARBITRARY) {
            eval(let->mutable_binds(i)->mutable_term(), env, backtrace_bind);
            unreachable("This term has type `TERM_TYPE_ARBITRARY`, so "
                "evaluating it must throw  `runtime_exc_t`.");
        }

        env->type_env.scope.put_in_scope(let->binds(i).var(),
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
            return env->scope.get(t->var());
            break;
        case Term::LET:
            {
                // Push the scope
                variable_val_scope_t::new_scope_t new_scope(&env->scope);
                variable_type_scope_t::new_scope_t new_type_scope(&env->type_env.scope);

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
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(t->number())));
            }
            break;
        case Term::STRING:
            {
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateString(t->valuestring().c_str())));
            }
            break;
        case Term::JSON:
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_Parse(t->jsonstring().c_str())));
            break;
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
                std::string pk = get_primary_key(t->get_by_key().table_ref().table_name(), env, backtrace);

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
            env->scope.dump(compiled ? NULL : &argnames, &argvals);

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
            if (env->implicit_attribute_value.has_value()) {
                object = env->implicit_attribute_value.get_value();
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
        case Term::VAR:
            crash("Eval stream should never be called on a var term (because"
                "streams can't be bound to vars) how did you get here and why"
                    "did you think this was a nice place to come to?");
            break;
        case Term::LET:
            {
                // Push the scope
                variable_val_scope_t::new_scope_t new_scope(&env->scope);
                variable_type_scope_t::new_scope_t new_type_scope(&env->type_env.scope);

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
        case Term::ERROR:
        case Term::NUMBER:
        case Term::STRING:
        case Term::JSON:
        case Term::BOOL:
        case Term::JSON_NULL:
        case Term::ARRAY:
        case Term::OBJECT:
        case Term::GETBYKEY:
        case Term::JAVASCRIPT:
            unreachable("eval_stream called on a function that does not return a stream (use eval instead).");
            break;
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
                    rassert(env->implicit_attribute_value.has_value());
                    data = env->implicit_attribute_value.get_value();
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
                    rassert(env->implicit_attribute_value.has_value());
                    data = env->implicit_attribute_value.get_value();
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
        case Builtin::PICKATTRS:
        case Builtin::IMPLICIT_PICKATTRS:
            {
                boost::shared_ptr<scoped_cJSON_t> data;
                if (c->builtin().type() == Builtin::PICKATTRS) {
                    data = eval(c->mutable_args(0), env, backtrace.with("arg:0"));
                } else {
                    rassert(env->implicit_attribute_value.has_value());
                    data = env->implicit_attribute_value.get_value();
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

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(left->DeepCopy()));

                // Extend with the right side (and overwrite if necessary)
                for(int i = 0; i < right->GetArraySize(); i++) {
                    cJSON *item = right->GetArrayItem(i);
                    res->DeleteItemFromObject(item->string);
                    res->AddItemToObject(item->string, cJSON_DeepCopy(item));
                }

                return res;
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
                env->scope.dump(&argnames, &values);

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

                    return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(result)));
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

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(result)));
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

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(result)));
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

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(result)));
                return res;
            }
            break;
        case Builtin::MODULO:
            {
                boost::shared_ptr<scoped_cJSON_t> lhs = eval_and_check(c->mutable_args(0), env, backtrace.with("arg:0"), cJSON_Number, "First operand of MOD must be a number."),
                                                  rhs = eval_and_check(c->mutable_args(1), env, backtrace.with("arg:1"), cJSON_Number, "Second operand of MOD must be a number.");

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(fmod(lhs->get()->valuedouble, rhs->get()->valuedouble))));
                return res;
            }
            break;
        case Builtin::COMPARE:
            {
                bool result = true;

                boost::shared_ptr<scoped_cJSON_t> lhs = eval(c->mutable_args(0), env, backtrace.with("arg:0"));

                for (int i = 1; i < c->args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> rhs = eval(c->mutable_args(i), env, backtrace.with(strprintf("arg:%d", i)));

                    int res = cJSON_cmp(lhs->get(), rhs->get(), backtrace.with(strprintf("arg:%d", i)));

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
        case Builtin::ARRAYTOSTREAM:
        case Builtin::GROUPEDMAPREDUCE:
        case Builtin::UNION:
        case Builtin::RANGE:
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

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(length)));
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
                // Start off accumulator with the base
                //boost::shared_ptr<scoped_cJSON_t> acc = eval(c->mutable_builtin()->mutable_reduce()->mutable_base(), env);

                //for (json_stream_t::iterator it  = stream.begin();
                //                             it != stream.end();
                //                             ++it)
                //{
                //    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                //    env->scope.put_in_scope(c->builtin().reduce().var1(), acc);
                //    env->scope.put_in_scope(c->builtin().reduce().var2(), *it);

                //    acc = eval(c->mutable_builtin()->mutable_reduce()->mutable_body(), env);
                //}
                //return acc;
            }
            break;
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
    variable_val_scope_t::new_scope_t scope_maker(&env.scope, pred.arg(), json);
    implicit_value_setter_t impliciter(&env.implicit_attribute_value, json);
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
    bool operator()(const boost::shared_ptr<scoped_cJSON_t> &x, const boost::shared_ptr<scoped_cJSON_t> &y) {
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
    variable_val_scope_t::new_scope_t scope_maker(&env.scope, arg, val);
    implicit_value_setter_t impliciter(&env.implicit_attribute_value, val);
    return eval(term, &env, backtrace);
}

boost::shared_ptr<json_stream_t> concatmap(std::string arg, Term *term, runtime_environment_t env, boost::shared_ptr<scoped_cJSON_t> val, const backtrace_t &backtrace) {
    variable_val_scope_t::new_scope_t scope_maker(&env.scope, arg, val);
    implicit_value_setter_t impliciter(&env.implicit_attribute_value, val);
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
            unreachable("eval_stream called on a function that does not return a stream (use eval instead).");
            break;
        case Builtin::GROUPEDMAPREDUCE:
            {
                Builtin::GroupedMapReduce *g = c->mutable_builtin()->mutable_grouped_map_reduce();
                shared_scoped_less_t comparator(backtrace);
                std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less_t> groups(comparator);
                boost::shared_ptr<json_stream_t> stream = eval_stream(c->mutable_args(0), env, backtrace.with("arg:0"));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    boost::shared_ptr<scoped_cJSON_t> group_mapped_row, value_mapped_row, reduced_row;

                    // Figure out which group we belong to
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scope, g->group_mapping().arg(), json);
                        implicit_value_setter_t impliciter(&env->implicit_attribute_value, json);
                        group_mapped_row = eval(g->mutable_group_mapping()->mutable_body(), env, backtrace.with("group_mapping"));
                    }

                    // Map the value for comfy reduction goodness
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scope, g->value_mapping().arg(), json);
                        implicit_value_setter_t impliciter(&env->implicit_attribute_value, json);
                        value_mapped_row = eval(g->mutable_value_mapping()->mutable_body(), env, backtrace.with("value_mapping"));
                    }

                    // Do the reduction
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                        std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less_t>::iterator
                            elem = groups.find(group_mapped_row);
                        if(elem != groups.end()) {
                            env->scope.put_in_scope(g->reduction().var1(),
                                                    (*elem).second);
                        } else {
                            env->scope.put_in_scope(g->reduction().var1(),
                                                    eval(g->mutable_reduction()->mutable_base(), env, backtrace.with("reduction").with("base")));
                        }
                        env->scope.put_in_scope(g->reduction().var2(), value_mapped_row);

                        reduced_row = eval(g->mutable_reduction()->mutable_body(), env, backtrace.with("reduction").with("body"));
                    }

                    // Phew, update the relevant group
                    groups[group_mapped_row] = reduced_row;

                }

                // Phew, convert the whole shebang to a stream
                // TODO this is an extra copy we just need a gracefull way to
                // deal with this
                std::list<boost::shared_ptr<scoped_cJSON_t> > res;

                std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less_t>::iterator it;
                for(it = groups.begin(); it != groups.end(); ++it) {
                    res.push_back((*it).second);
                }

                return boost::shared_ptr<in_memory_stream_t>(new in_memory_stream_t(res.begin(), res.end(), env));
            }
            break;
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
                }

                if (r->has_upperbound()) {
                    upperbound = eval(r->mutable_upperbound(), env, backtrace.with("upperbound"));
                }

                if (lowerbound && upperbound) {
                    range = key_range_t(key_range_t::closed, store_key_t(lowerbound->Print()),
                                        key_range_t::closed, store_key_t(upperbound->Print()));
                } else if (lowerbound) {
                    range = key_range_t(key_range_t::closed, store_key_t(lowerbound->Print()),
                                        key_range_t::none, store_key_t());
                } else if (upperbound) {
                    range = key_range_t(key_range_t::none, store_key_t(),
                                        key_range_t::closed, store_key_t(upperbound->Print()));
                }

                return boost::shared_ptr<json_stream_t>(new range_stream_t(stream, range, r->attrname()));
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
    TableRef *t, runtime_environment_t *env, const backtrace_t &backtrace)
    THROWS_ONLY(runtime_exc_t) {
    boost::optional<std::pair<namespace_id_t, deletable_t< namespace_semilattice_metadata_t<rdb_protocol_t> > > > namespace_info =
        metadata_get_by_name(env->semilattice_metadata->get().rdb_namespaces.namespaces, t->table_name());
    if (namespace_info) {
        return namespace_repo_t<rdb_protocol_t>::access_t(
            env->ns_repo, namespace_info->first, env->interruptor);
    } else {
        throw runtime_exc_t(strprintf("Namespace %s either not found, ambiguous or namespace metadata in conflict.", t->table_name().c_str()), backtrace);
    }
}

view_t eval_view(Term::Call *c, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (c->builtin().type()) {
        //JSON -> JSON
        case Builtin::NOT:
        case Builtin::GETATTR:
        case Builtin::IMPLICIT_GETATTR:
        case Builtin::HASATTR:
        case Builtin::IMPLICIT_HASATTR:
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
    std::string pk = get_primary_key(t->mutable_table_ref()->table_name(), env, backtrace);
    boost::shared_ptr<json_stream_t> stream(new batched_rget_stream_t(ns_access, env->interruptor, key_range_t::universe(), 100, backtrace));
    return view_t(ns_access, pk, stream);
}

} //namespace query_language
