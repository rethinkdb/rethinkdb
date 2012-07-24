#include "rdb_protocol/query_language.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <math.h>

#include "http/json.hpp"
#include "rdb_protocol/js.hpp"

namespace query_language {

void check_protobuf(bool cond) {
    if (!cond) {
        throw bad_protobuf_exc_t();
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

bool term_type_least_upper_bound(term_type_t left, term_type_t right, term_type_t *out) {
    if (term_type_is_convertible(left, TERM_TYPE_ARBITRARY) &&
            term_type_is_convertible(right, TERM_TYPE_ARBITRARY)) {
        *out = TERM_TYPE_ARBITRARY;
        return true;
    } else if (term_type_is_convertible(left, TERM_TYPE_JSON) &&
            term_type_is_convertible(right, TERM_TYPE_JSON)) {
        *out = TERM_TYPE_JSON;
        return true;
    } else if (term_type_is_convertible(left, TERM_TYPE_VIEW) &&
            term_type_is_convertible(right, TERM_TYPE_VIEW)) {
        *out = TERM_TYPE_VIEW;
        return true;
    } else if (term_type_is_convertible(left, TERM_TYPE_STREAM) &&
            term_type_is_convertible(right, TERM_TYPE_STREAM)) {
        *out = TERM_TYPE_STREAM;
        return true;
    } else {
        return false;
    }
}

term_type_t get_term_type(const Term &t, type_checking_environment_t *env, const backtrace_t &backtrace) {
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
            term_type_t res = get_term_type(t.let().expr(), env, backtrace.with("expr"));
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
            check_term_type(t.if_().test(), TERM_TYPE_JSON, env, backtrace.with("test"));

            term_type_t true_branch = get_term_type(t.if_().true_branch(), env, backtrace.with("true_branch"));

            term_type_t false_branch = get_term_type(t.if_().false_branch(), env, backtrace.with("false_branch"));

            term_type_t combined_type;
            if (!term_type_least_upper_bound(true_branch, false_branch, &combined_type)) {
                throw bad_query_exc_t(strprintf(
                    "true-branch has type %s, but false-branch has type %s",
                    term_type_name(true_branch).c_str(),
                    term_type_name(false_branch).c_str()
                    ),
                    backtrace);
            }
            return combined_type;
        }
        break;
    case Term::ERROR:
        check_protobuf(t.has_error());
        return TERM_TYPE_ARBITRARY;
        break;
    case Term::NUMBER:
        check_protobuf(t.has_number());
        return TERM_TYPE_JSON;
        break;
    case Term::STRING:
        check_protobuf(t.has_valuestring());
        return TERM_TYPE_JSON;
        break;
    case Term::JSON:
        check_protobuf(t.has_jsonstring());
        return TERM_TYPE_JSON;
        break;
    case Term::BOOL:
        check_protobuf(t.has_valuebool());
        return TERM_TYPE_JSON;
        break;
    case Term::JSON_NULL:
        check_protobuf(field_count == 1); // null term has only a type field
        return TERM_TYPE_JSON;
        break;
    case Term::ARRAY:
        if (t.array_size() == 0) { // empty arrays are valid
            check_protobuf(field_count == 1);
        }
        for (int i = 0; i < t.array_size(); ++i) {
            check_term_type(t.array(i), TERM_TYPE_JSON, env, backtrace.with(strprintf("elem:%d", i)));
        }
        return TERM_TYPE_JSON;
        break;
    case Term::OBJECT:
        if (t.object_size() == 0) { // empty objects are valid
            check_protobuf(field_count == 1);
        }
        for (int i = 0; i < t.object_size(); ++i) {
            check_term_type(t.object(i).term(), TERM_TYPE_JSON, env,
                backtrace.with(strprintf("key:%s", t.object(i).var().c_str())));
        }
        return TERM_TYPE_JSON;
        break;
    case Term::GETBYKEY: {
        check_protobuf(t.has_get_by_key());
        check_term_type(t.get_by_key().key(), TERM_TYPE_JSON, env, backtrace.with("key"));
        return TERM_TYPE_JSON;
        break;
    }
    case Term::TABLE:
        check_protobuf(t.has_table());
        return TERM_TYPE_VIEW;
        break;
    case Term::JAVASCRIPT:
        check_protobuf(t.has_javascript());
        return TERM_TYPE_JSON;
        break;
    default:
        unreachable("unhandled Term case");
    }
    crash("unreachable");
}

void check_term_type(const Term &t, term_type_t expected, type_checking_environment_t *env, const backtrace_t &backtrace) {
    term_type_t actual = get_term_type(t, env, backtrace);
    if (!term_type_is_convertible(actual, expected)) {
        throw bad_query_exc_t(strprintf("expected a %s; got a %s",
                term_type_name(expected).c_str(), term_type_name(actual).c_str()),
            backtrace);
    }
}

void check_arg_count(const Term::Call &c, int n_args, const backtrace_t &backtrace) {
    if (c.args_size() != n_args) {
        const char* fn_name = Builtin::BuiltinType_Name(c.builtin().type()).c_str();
        throw bad_query_exc_t(strprintf(
            "%s takes %d argument%s (%d given)",
            fn_name,
            n_args,
            n_args > 1 ? "s" : "",
            c.args_size()
            ),
            backtrace);
    }
}

void check_function_args(const Term::Call &c, const term_type_t &arg_type, int n_args,
                         type_checking_environment_t *env, const backtrace_t &backtrace) {
    check_arg_count(c, n_args, backtrace);
    for (int i = 0; i < c.args_size(); ++i) {
        check_term_type(c.args(i), arg_type, env, backtrace.with(strprintf("arg:%d", i)));
    }
}

void check_function_args(const Term::Call &c, const term_type_t &arg1_type, const term_type_t &arg2_type,
                         type_checking_environment_t *env, const backtrace_t &backtrace) {
    check_arg_count(c, 2, backtrace);
    check_term_type(c.args(0), arg1_type, env, backtrace.with("arg:0"));
    check_term_type(c.args(1), arg2_type, env, backtrace.with("arg:0"));
}

term_type_t get_function_type(const Term::Call &c, type_checking_environment_t *env, const backtrace_t &backtrace) {
    std::vector<const google::protobuf::FieldDescriptor *> fields;

    const Builtin &b = c.builtin();

    b.GetReflection()->ListFields(b, &fields);

    int field_count = fields.size();

    check_protobuf(field_count <= 2);

    // this is a bit cleaner when we check well-formedness separate
    // from returning the type

    switch (c.builtin().type()) {
    case Builtin::NOT:
    case Builtin::MAPMERGE:
    case Builtin::ARRAYAPPEND:
    case Builtin::ARRAYCONCAT:
    case Builtin::ARRAYSLICE:
    case Builtin::ARRAYNTH:
    case Builtin::ARRAYLENGTH:
    case Builtin::ADD:
    case Builtin::SUBTRACT:
    case Builtin::MULTIPLY:
    case Builtin::DIVIDE:
    case Builtin::MODULO:
    case Builtin::DISTINCT:
    case Builtin::LIMIT:
    case Builtin::LENGTH:
    case Builtin::UNION:
    case Builtin::NTH:
    case Builtin::STREAMTOARRAY:
    case Builtin::ARRAYTOSTREAM:
    case Builtin::ANY:
    case Builtin::ALL:
        // these builtins only have
        // Builtin.type set
        check_protobuf(field_count == 1);
        break;
    case Builtin::COMPARE:
        check_protobuf(b.has_comparison());
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
        implicit_value_t<term_type_t>::impliciter_t impliciter(&env->implicit_type, TERM_TYPE_JSON); //make the implicit value be of type json
        check_protobuf(b.has_filter());
        check_predicate_type(b.filter().predicate(), env, backtrace.with("predicate"));
        break;
    }
    case Builtin::MAP: {
        implicit_value_t<term_type_t>::impliciter_t impliciter(&env->implicit_type, TERM_TYPE_JSON); //make the implicit value be of type json
        check_protobuf(b.has_map());
        check_mapping_type(b.map().mapping(), TERM_TYPE_JSON, env, backtrace.with("mapping"));
        break;
    }
    case Builtin::CONCATMAP: {
        implicit_value_t<term_type_t>::impliciter_t impliciter(&env->implicit_type, TERM_TYPE_JSON); //make the implicit value be of type json
        check_protobuf(b.has_concat_map());
        check_mapping_type(b.concat_map().mapping(), TERM_TYPE_STREAM, env, backtrace.with("mapping"));
        break;
    }
    case Builtin::ORDERBY:
        check_protobuf(b.order_by_size() > 0);
        break;
    case Builtin::REDUCE: {
        implicit_value_t<term_type_t>::impliciter_t impliciter(&env->implicit_type, TERM_TYPE_JSON); //make the implicit value be of type json
        check_protobuf(b.has_reduce());
        check_reduction_type(b.reduce(), env, backtrace.with("reduce"));
        break;
    }
    case Builtin::GROUPEDMAPREDUCE: {
        check_protobuf(b.has_grouped_map_reduce());
        implicit_value_t<term_type_t>::impliciter_t impliciter(&env->implicit_type, TERM_TYPE_JSON); //make the implicit value be of type json
        check_mapping_type(b.grouped_map_reduce().group_mapping(), TERM_TYPE_JSON, env, backtrace.with("group_mapping"));
        check_mapping_type(b.grouped_map_reduce().value_mapping(), TERM_TYPE_JSON, env, backtrace.with("value_mapping"));
        check_reduction_type(b.grouped_map_reduce().reduction(), env, backtrace.with("reduction"));
        break;
    }
    case Builtin::RANGE:
        check_protobuf(b.has_range());
        if (b.range().has_lowerbound()) {
            check_term_type(b.range().lowerbound(), TERM_TYPE_JSON, env, backtrace.with("lowerbound"));
        }
        if (b.range().has_upperbound()) {
            check_term_type(b.range().upperbound(), TERM_TYPE_JSON, env, backtrace.with("upperbound"));
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
        case Builtin::ARRAYLENGTH:
            check_function_args(c, TERM_TYPE_JSON, 1, env, backtrace);
            return TERM_TYPE_JSON;
            break;
        case Builtin::IMPLICIT_GETATTR:
        case Builtin::IMPLICIT_HASATTR:
        case Builtin::IMPLICIT_PICKATTRS:
            check_arg_count(c, 0, backtrace);
            if (!env->implicit_type.has_value() || env->implicit_type.get_value() != TERM_TYPE_JSON) {
                throw bad_query_exc_t("No implicit variable in scope", backtrace);
            }
            return TERM_TYPE_JSON;
            break;
        case Builtin::MAPMERGE:
        case Builtin::ARRAYAPPEND:
        case Builtin::ARRAYCONCAT:
        case Builtin::ARRAYNTH:
        case Builtin::MODULO:
            check_function_args(c, TERM_TYPE_JSON, 2, env, backtrace);
            return TERM_TYPE_JSON;
            break;
        case Builtin::ADD:
        case Builtin::SUBTRACT:
        case Builtin::MULTIPLY:
        case Builtin::DIVIDE:
        case Builtin::COMPARE:
        case Builtin::ANY:
        case Builtin::ALL:
            check_function_args(c, TERM_TYPE_JSON, c.args_size(), env, backtrace);
            return TERM_TYPE_JSON;  // variadic JSON type
            break;
        case Builtin::ARRAYSLICE:
            check_function_args(c, TERM_TYPE_JSON, 3, env, backtrace);
            return TERM_TYPE_JSON;
            break;
        case Builtin::MAP:
        case Builtin::CONCATMAP:
        case Builtin::DISTINCT:
            check_function_args(c, TERM_TYPE_STREAM, 1, env, backtrace);
            return TERM_TYPE_STREAM;
            break;
        case Builtin::FILTER:
        case Builtin::ORDERBY:
            check_function_args(c, TERM_TYPE_STREAM, 1, env, backtrace);
            // polymorphic
            return get_term_type(c.args(0), env, backtrace);
            break;
        case Builtin::LIMIT:
            check_function_args(c, TERM_TYPE_STREAM, TERM_TYPE_JSON, env, backtrace);
            // polymorphic: view -> view, stream -> stream
            return get_term_type(c.args(0), env, backtrace);
            break;
        case Builtin::NTH:
            check_function_args(c, TERM_TYPE_STREAM, TERM_TYPE_JSON, env, backtrace);
            return TERM_TYPE_JSON;
            break;
        case Builtin::LENGTH:
        case Builtin::STREAMTOARRAY:
        case Builtin::REDUCE:
        case Builtin::GROUPEDMAPREDUCE:
            check_function_args(c, TERM_TYPE_STREAM, 1, env, backtrace);
            return TERM_TYPE_JSON;
            break;
        case Builtin::UNION:
            check_function_args(c, TERM_TYPE_STREAM, 2, env, backtrace);
            return TERM_TYPE_STREAM;
            break;
        case Builtin::ARRAYTOSTREAM:
            check_function_args(c, TERM_TYPE_JSON, 1, env, backtrace);
            return TERM_TYPE_STREAM;
            break;
        case Builtin::RANGE:
            check_function_args(c, TERM_TYPE_STREAM, 1, env, backtrace);
            return TERM_TYPE_STREAM;
            break;
        default:
            crash("unreachable");
            break;
    }

    crash("unreachable");
}

void check_reduction_type(const Reduction &r, type_checking_environment_t *env, const backtrace_t &backtrace) {
    check_term_type(r.base(), TERM_TYPE_JSON, env, backtrace.with("base"));

    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(r.var1(), TERM_TYPE_JSON);
    env->scope.put_in_scope(r.var2(), TERM_TYPE_JSON);
    check_term_type(r.body(), TERM_TYPE_JSON, env, backtrace.with("body"));
}

void check_mapping_type(const Mapping &m, term_type_t return_type, type_checking_environment_t *env, const backtrace_t &backtrace) {
    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(m.arg(), TERM_TYPE_JSON);
    check_term_type(m.body(), return_type, env, backtrace);
}

void check_predicate_type(const Predicate &p, type_checking_environment_t *env, const backtrace_t &backtrace) {
    new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(p.arg(), TERM_TYPE_JSON);
    check_term_type(p.body(), TERM_TYPE_JSON, env, backtrace);
}

void check_read_query_type(const ReadQuery &rq, type_checking_environment_t *env, const backtrace_t &backtrace) {
    /* Read queries could return anything--a view, a stream, a JSON, or an
    error. Views will be automatically converted to streams at evaluation time.
    */
    get_term_type(rq.term(), env, backtrace);
}

void check_write_query_type(const WriteQuery &w, type_checking_environment_t *env, const backtrace_t &backtrace) {
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    w.GetReflection()->ListFields(w, &fields);
    check_protobuf(fields.size() == 2);

    switch (w.type()) {
        case WriteQuery::UPDATE: {
            check_protobuf(w.has_update());
            check_term_type(w.update().view(), TERM_TYPE_VIEW, env, backtrace.with("view"));
            check_mapping_type(w.update().mapping(), TERM_TYPE_JSON, env, backtrace.with("mapping"));
            break;
        }
        case WriteQuery::DELETE: {
            check_protobuf(w.has_delete_());
            check_term_type(w.delete_().view(), TERM_TYPE_VIEW, env, backtrace.with("view"));
            break;
        }
        case WriteQuery::MUTATE: {
            check_protobuf(w.has_mutate());
            check_term_type(w.mutate().view(), TERM_TYPE_VIEW, env, backtrace.with("view"));
            check_mapping_type(w.mutate().mapping(), TERM_TYPE_JSON, env, backtrace.with("mapping"));
            break;
        }
        case WriteQuery::INSERT: {
            check_protobuf(w.has_insert());
            for (int i = 0; i < w.insert().terms_size(); ++i) {
                check_term_type(w.insert().terms(i), TERM_TYPE_JSON, env, backtrace.with(strprintf("term:%d", i)));
            }
            break;
        }
        case WriteQuery::INSERTSTREAM: {
            check_protobuf(w.has_insert_stream());
            check_term_type(w.insert_stream().stream(), TERM_TYPE_STREAM, env, backtrace.with("stream"));
            break;
        }
        case WriteQuery::FOREACH:
            {
                check_protobuf(w.has_for_each());
                check_term_type(w.for_each().stream(), TERM_TYPE_STREAM, env, backtrace.with("stream"));

                new_scope_t scope_maker(&env->scope);
                env->scope.put_in_scope(w.for_each().var(), TERM_TYPE_JSON);
                for (int i = 0; i < w.for_each().queries_size(); ++i) {
                    check_write_query_type(w.for_each().queries(i), env, backtrace.with(strprintf("query:%d", i)));
                }
            }
            break;
        case WriteQuery::POINTUPDATE: {
            check_protobuf(w.has_point_update());
            check_term_type(w.point_update().key(), TERM_TYPE_JSON, env, backtrace.with("key"));
            check_mapping_type(w.point_update().mapping(), TERM_TYPE_JSON, env, backtrace.with("mapping"));
            break;
        }
        case WriteQuery::POINTDELETE: {
            check_protobuf(w.has_point_delete());
            check_term_type(w.point_delete().key(), TERM_TYPE_JSON, env, backtrace.with("key"));
            break;
        }
        case WriteQuery::POINTMUTATE: {
            check_protobuf(w.has_point_mutate());
            check_term_type(w.point_mutate().key(), TERM_TYPE_JSON, env, backtrace.with("key"));
            check_mapping_type(w.point_mutate().mapping(), TERM_TYPE_JSON, env, backtrace.with("mapping"));
            break;
        }
        default:
            unreachable("unhandled WriteQuery");
    }
}

void check_query_type(const Query &q, type_checking_environment_t *env, const backtrace_t &backtrace) {
    switch (q.type()) {
    case Query::READ:
        check_protobuf(q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_read_query_type(q.read_query(), env, backtrace);
        break;
    case Query::WRITE:
        check_protobuf(q.has_write_query());
        check_protobuf(!q.has_read_query());
        check_write_query_type(q.write_query(), env, backtrace);
        break;
    default:
        unreachable("unhandled Query");
    }
}

int cJSON_cmp(cJSON *l, cJSON *r, const backtrace_t &backtrace) {
    if (l->type != r->type) {
        return l->type - r->type;
    }
    switch (l->type) {
        case cJSON_False:
            if (r->type == cJSON_True) {
                return -1;
            } else if (r->type == cJSON_False) {
                return 0;
            } else {
                throw runtime_exc_t("Booleans can only be compared to other booleans", backtrace);
            }
            break;
        case cJSON_True:
            if (r->type == cJSON_True) {
                return 0;
            } else if (r->type == cJSON_False) {
                return 1;
            } else {
                throw runtime_exc_t("Booleans can only be compared to other booleans", backtrace);
            }
            break;
        case cJSON_NULL:
            return 1;
            break;
        case cJSON_Number:
            if (r->type != cJSON_Number) {
                throw runtime_exc_t("Numbers can only be compared to other numbers.", backtrace);
            }
            if (l->valuedouble < r->valuedouble) {
                return -1;
            } else if (l->valuedouble > r->valuedouble) {
                return 1;
            } else {
                return 0;   // TODO: Handle NaN?
            }
            break;
        case cJSON_String:
            if (r->type != cJSON_String) {
                throw runtime_exc_t("Strings can only be compared to other strings.", backtrace);
            }
            return strcmp(l->valuestring, r->valuestring) < 0;
            break;
        case cJSON_Array:
            if (r->type == cJSON_Array) {
                int lsize = cJSON_GetArraySize(l),
                    rsize = cJSON_GetArraySize(r);
                for (int i = 0; i < lsize; ++i) {
                    if (i >= rsize) {
                        return 1;  // e.g. cmp([0, 1], [0])
                    }
                    int cmp = cJSON_cmp(cJSON_GetArrayItem(l, i), cJSON_GetArrayItem(r, i), backtrace);
                    if (cmp) {
                        return cmp;
                    }
                }
                return -1;  // e.g. cmp([0], [0, 1]);
            } else {
                throw runtime_exc_t("Strings can only be compared to other strings.", backtrace);
            }
            break;
        case cJSON_Object:
            throw runtime_exc_t("Can't compare objects.", backtrace);
            break;
        default:
            unreachable();
            break;
    }
}

class shared_scoped_less {
public:
    shared_scoped_less(const backtrace_t &bt) : backtrace(bt) { }
    bool operator()(const boost::shared_ptr<scoped_cJSON_t> &a,
                      const boost::shared_ptr<scoped_cJSON_t> &b) {
        if (a->type() == b->type()) {
            return cJSON_cmp(a->get(), b->get(), backtrace) < 0;
        } else {
            return a->type() > b->type();
        }
    }
private:
    backtrace_t backtrace;
};

boost::shared_ptr<js::runner_t> runtime_environment_t::get_js_runner() {
    if (!js_runner->begun()) {
        pool->assert_thread();
        js_runner->begin(pool);
    }
    return js_runner;
}

void execute(const Query &q, runtime_environment_t *env, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    if (q.type() == Query::READ) {
        execute(q.read_query(), env, res, backtrace);
    } else if (q.type() == Query::WRITE) {
        execute(q.write_query(), env, res, backtrace);
    } else {
        crash("unreachable");
    }
}

void execute(const ReadQuery &r, runtime_environment_t *env, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    term_type_t type = get_term_type(r.term(), &env->type_env, backtrace);

    switch (type) {
    case TERM_TYPE_JSON: {
        boost::shared_ptr<scoped_cJSON_t> json = eval(r.term(), env, backtrace);
        res->add_response(json->PrintUnformatted());
        break;
    }
    case TERM_TYPE_STREAM:
    case TERM_TYPE_VIEW: {
        boost::shared_ptr<json_stream_t> stream = eval_stream(r.term(), env, backtrace);
        while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            res->add_response(json->PrintUnformatted());
        }
        break;
    }
    case TERM_TYPE_ARBITRARY: {
        eval(r.term(), env, backtrace);
        unreachable("This term has type `TERM_TYPE_ARBITRARY`, so evaluating "
            "it should throw `runtime_exc_t`.");
    }
    default:
        unreachable("read query type invalid.");
    }
}

void insert(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> data, runtime_environment_t *env, const backtrace_t &backtrace) {
    if (!data->GetObjectItem("id")) {
        throw runtime_exc_t("Must have a field named id.", backtrace);
    }

    try {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t(cJSON_print_std_string(data->GetObjectItem("id"))), data));
        ns_access.get_namespace_if()->write(write, order_token_t::ignore, env->interruptor);
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, cJSON *id, runtime_environment_t *env, const backtrace_t &backtrace) {
    try {
        rdb_protocol_t::write_t write(rdb_protocol_t::point_delete_t(store_key_t(cJSON_print_std_string(id))));
        ns_access.get_namespace_if()->write(write, order_token_t::ignore, env->interruptor);
    } catch (cannot_perform_query_exc_t e) {
        throw runtime_exc_t("cannot perform write: " + std::string(e.what()), backtrace);
    }
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> id, runtime_environment_t *env, const backtrace_t &backtrace) {
    point_delete(ns_access, id->get(), env, backtrace);
}

void execute(const WriteQuery &w, runtime_environment_t *env, Response *res, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (w.type()) {
        case WriteQuery::UPDATE:
            {
                view_t view = eval_view(w.update().view(), env, backtrace.with("view"));

                int updated = 0,
                    error = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope);

                    env->scope.put_in_scope(w.update().mapping().arg(), json);
                    boost::shared_ptr<scoped_cJSON_t> val = eval(w.update().mapping().body(), env, backtrace.with("mapping"));
                    if (!cJSON_Equal(json->GetObjectItem("id"),
                                     val->GetObjectItem("id"))) {
                        error++;
                    } else {
                        insert(view.access, val, env, backtrace);
                        updated++;
                    }
                }

                res->add_response(strprintf("{\"updated\": %d, \"errors\": %d}", updated, error));
            }
            break;
        case WriteQuery::DELETE:
            {
                view_t view = eval_view(w.delete_().view(), env, backtrace.with("view"));

                int deleted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    point_delete(view.access, json->GetObjectItem("id"), env, backtrace);
                    deleted++;
                }

                res->add_response(strprintf("{\"deleted\": %d}", deleted));
            }
            break;
        case WriteQuery::MUTATE:
            {
                view_t view = eval_view(w.update().view(), env, backtrace.with("view"));

                int modified = 0, deleted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                    env->scope.put_in_scope(w.update().mapping().arg(), json);
                    boost::shared_ptr<scoped_cJSON_t> val = eval(w.update().mapping().body(), env, backtrace.with("mapping"));

                    if (val->type() == cJSON_NULL) {
                        point_delete(view.access, json->GetObjectItem("id"), env, backtrace);
                        ++deleted;
                    } else {
                        insert(view.access, val, env, backtrace);
                        ++modified;
                    }
                }

                res->add_response(strprintf("{\"modified\": %d, \"deleted\": %d}", modified, deleted));
            }
            break;
        case WriteQuery::INSERT:
            {
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.insert().table_ref(), env, backtrace);
                for (int i = 0; i < w.insert().terms_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> data = eval(w.insert().terms(i), env,
                        backtrace.with(strprintf("term:%d", i)));

                    insert(ns_access, data, env, backtrace.with(strprintf("term:%d", i)));
                }

                res->add_response(strprintf("{\"inserted\": %d}", w.insert().terms_size()));
            }
            break;
        case WriteQuery::INSERTSTREAM:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(w.insert_stream().stream(), env, backtrace.with("stream"));

                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.insert_stream().table_ref(), env, backtrace);

                int inserted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    inserted++;
                    insert(ns_access, json, env, backtrace);
                }

                res->add_response(strprintf("{\"inserted\": %d}", inserted));
            }
            break;
        case WriteQuery::FOREACH:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(w.for_each().stream(), env, backtrace.with("stream"));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                    env->scope.put_in_scope(w.for_each().var(), json);

                    for (int i = 0; i < w.for_each().queries_size(); ++i) {
                        execute(w.for_each().queries(i), env, res, backtrace.with(strprintf("query:%d", i)));
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
                *get_by_key.mutable_table_ref() = w.point_update().table_ref();
                get_by_key.set_attrname(w.point_update().attrname());
                *get_by_key.mutable_key() = w.point_update().key();
                *get.mutable_get_by_key() = get_by_key;

                boost::shared_ptr<scoped_cJSON_t> original_val = eval(get, env, backtrace);
                new_val_scope_t scope_maker(&env->scope);
                env->scope.put_in_scope(w.point_update().mapping().arg(), original_val);

                boost::shared_ptr<scoped_cJSON_t> new_val = eval(w.point_update().mapping().body(), env, backtrace.with("mapping"));

                /* Now we insert the new value. */
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.point_update().table_ref(), env, backtrace);

                insert(ns_access, new_val, env, backtrace);

                res->add_response(strprintf("{\"updated\": %d, \"errors\": %d}", 1, 0));
            }
            break;
        case WriteQuery::POINTDELETE:
            {
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.point_delete().table_ref(), env, backtrace);
                boost::shared_ptr<scoped_cJSON_t> id = eval(w.point_delete().key(), env, backtrace.with("key"));
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
void eval_let_binds(const Term::Let &let, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    // Go through the bindings in a let and add them one by one
    for (int i = 0; i < let.binds_size(); ++i) {
        backtrace_t backtrace_bind = backtrace.with(strprintf("bind:%s", let.binds(i).var().c_str()));
        term_type_t type = get_term_type(let.binds(i).term(), &env->type_env, backtrace_bind);

        if (type == TERM_TYPE_JSON) {
            env->scope.put_in_scope(let.binds(i).var(),
                    eval(let.binds(i).term(), env, backtrace_bind));
        } else if (type == TERM_TYPE_STREAM || type == TERM_TYPE_VIEW) {
            env->stream_scope.put_in_scope(let.binds(i).var(),
                    boost::shared_ptr<stream_multiplexer_t>(new stream_multiplexer_t(eval_stream(let.binds(i).term(), env, backtrace_bind))));
        } else if (type == TERM_TYPE_ARBITRARY) {
            eval(let.binds(i).term(), env, backtrace_bind);
            unreachable("This term has type `TERM_TYPE_ARBITRARY`, so "
                "evaluating it must throw  `runtime_exc_t`.");
        }

        env->type_env.scope.put_in_scope(let.binds(i).var(),
                                     type);
    }
}

boost::shared_ptr<scoped_cJSON_t> eval(const Term &t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (t.type()) {
        case Term::VAR:
            return env->scope.get(t.var());
            break;
        case Term::LET:
            {
                // Push the scope
                variable_val_scope_t::new_scope_t new_scope(&env->scope);
                variable_stream_scope_t::new_scope_t new_stream_scope(&env->stream_scope);
                variable_type_scope_t::new_scope_t new_type_scope(&env->type_env.scope);

                eval_let_binds(t.let(), env, backtrace);

                return eval(t.let().expr(), env, backtrace.with("expr"));
            }
            break;
        case Term::CALL:
            return eval(t.call(), env, backtrace);
            break;
        case Term::IF:
            {
                boost::shared_ptr<scoped_cJSON_t> test = eval(t.if_().test(), env, backtrace.with("test"));
                if (test->type() != cJSON_True && test->type() != cJSON_False) {
                    throw runtime_exc_t("The IF test must evaluate to a boolean.", backtrace.with("test"));
                }

                boost::shared_ptr<scoped_cJSON_t> res;
                if (test->type() == cJSON_True) {
                    res = eval(t.if_().true_branch(), env, backtrace.with("true"));
                } else {
                    res = eval(t.if_().false_branch(), env, backtrace.with("false"));
                }
                return res;
            }
            break;
        case Term::ERROR:
            throw runtime_exc_t(t.error(), backtrace);
            break;
        case Term::NUMBER:
            {
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(t.number())));
            }
            break;
        case Term::STRING:
            {
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateString(t.valuestring().c_str())));
            }
            break;
        case Term::JSON:
            return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_Parse(t.jsonstring().c_str())));
            break;
        case Term::BOOL:
            {
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateBool(t.valuebool())));
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
                for (int i = 0; i < t.array_size(); ++i) {
                    res->AddItemToArray(eval(t.array(i), env, backtrace.with(strprintf("elem:%d", i)))->release());
                }
                return res;
            }
            break;
        case Term::OBJECT:
            {
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateObject()));
                for (int i = 0; i < t.object_size(); ++i) {
                    std::string item_name(t.object(i).var());
                    res->AddItemToObject(item_name.c_str(), eval(t.object(i).term(), env, backtrace.with(strprintf("key:%s", item_name.c_str())))->release());
                }
                return res;
            }
            break;
        case Term::GETBYKEY:
            {
                boost::optional<std::pair<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<rdb_protocol_t> > > > namespace_info =
                    env->semilattice_metadata->get().rdb_namespaces.get_namespace_by_name(t.get_by_key().table_ref().table_name());

                if (!namespace_info) {
                    throw runtime_exc_t(strprintf("Namespace %s either not found, ambigious or namespace metadata in conflict.", t.get_by_key().table_ref().table_name().c_str()),
                        backtrace.with("table_ref"));
                }

                if (t.get_by_key().attrname() != "id") {
                    throw runtime_exc_t(strprintf("Attribute: %s is not the primary key and thus cannot be selected upon.", t.get_by_key().attrname().c_str()),
                        backtrace.with("attrname"));
                }

                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(t.get_by_key().table_ref(), env, backtrace);

                boost::shared_ptr<scoped_cJSON_t> key = eval(t.get_by_key().key(), env, backtrace.with("key"));

                try {
                    rdb_protocol_t::read_t read(rdb_protocol_t::point_read_t(store_key_t(key->Print())));
                    rdb_protocol_t::read_response_t res = ns_access.get_namespace_if()->read(read, order_token_t::ignore, env->interruptor);

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
            boost::shared_ptr<js::runner_t> js = env->get_js_runner();
            std::string errmsg;
            boost::shared_ptr<scoped_cJSON_t> result;

            // TODO(rntz): set up a js::runner_t::req_config_t with an
            // appropriately-chosen timeout.
            {
                // Construct an environment mapping.
                // TODO (rntz): making a new map for this is wasteful.
                std::map<std::string, boost::shared_ptr<scoped_cJSON_t> > context;
                env->scope.dump(&context);

                // Evaluate the source.
                rassert(t.has_javascript());
                result = js->eval(t.javascript().c_str(), t.javascript().size(), context, &errmsg);
            }

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

boost::shared_ptr<json_stream_t> eval_stream(const Term &t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (t.type()) {
        case Term::VAR:
            return boost::shared_ptr<json_stream_t>(new stream_multiplexer_t::stream_t(env->stream_scope.get(t.var())));
            break;
        case Term::LET:
            {
                // Push the scope
                variable_val_scope_t::new_scope_t new_scope(&env->scope);
                variable_stream_scope_t::new_scope_t new_stream_scope(&env->stream_scope);
                variable_type_scope_t::new_scope_t new_type_scope(&env->type_env.scope);

                eval_let_binds(t.let(), env, backtrace);

                return eval_stream(t.let().expr(), env, backtrace.with("expr"));
            }
            break;
        case Term::CALL:
            return eval_stream(t.call(), env, backtrace);
            break;
        case Term::IF:
            {
                boost::shared_ptr<scoped_cJSON_t> test = eval(t.if_().test(), env, backtrace.with("test"));
                if (test->type() != cJSON_True && test->type() != cJSON_False) {
                    throw runtime_exc_t("The IF test must evaluate to a boolean.", backtrace.with("test"));
                }

                if (test->type() == cJSON_True) {
                    return eval_stream(t.if_().true_branch(), env, backtrace.with("true"));
                } else {
                    return eval_stream(t.if_().false_branch(), env, backtrace.with("false"));
                }
            }
            break;
        case Term::TABLE:
            {
                return eval_view(t.table(), env, backtrace).stream;
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

boost::shared_ptr<scoped_cJSON_t> eval(const Term::Call &c, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (c.builtin().type()) {
        //JSON -> JSON
        case Builtin::NOT:
            {
                boost::shared_ptr<scoped_cJSON_t> data = eval(c.args(0), env, backtrace.with("arg:0"));

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
                if (c.builtin().type() == Builtin::GETATTR) {
                    data = eval(c.args(0), env, backtrace.with("arg:0"));
                } else {
                    rassert(env->implicit_attribute_value.has_value());
                    data = env->implicit_attribute_value.get_value();
                }

                if (!data->type() == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object", backtrace.with("arg:0"));
                }

                cJSON *value = data->GetObjectItem(c.builtin().attr().c_str());

                if (!value) {
                    throw runtime_exc_t("Object is missing attribute \"" + c.builtin().attr() + "\"", backtrace.with("attr"));
                }

                return shared_scoped_json(cJSON_DeepCopy(value));
            }
            break;
        case Builtin::HASATTR:
        case Builtin::IMPLICIT_HASATTR:
            {
                boost::shared_ptr<scoped_cJSON_t> data;
                if (c.builtin().type() == Builtin::HASATTR) {
                    data = eval(c.args(0), env, backtrace.with("arg:0"));
                } else {
                    rassert(env->implicit_attribute_value.has_value());
                    data = env->implicit_attribute_value.get_value();
                }

                if (!data->type() == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object", backtrace.with("arg:0"));
                }

                cJSON *attr = data->GetObjectItem(c.builtin().attr().c_str());

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
                if (c.builtin().type() == Builtin::PICKATTRS) {
                    data = eval(c.args(0), env, backtrace.with("arg:0"));
                } else {
                    rassert(env->implicit_attribute_value.has_value());
                    data = env->implicit_attribute_value.get_value();
                }

                if (!data->type() == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object", backtrace.with("arg:0"));
                }

                boost::shared_ptr<scoped_cJSON_t> res = shared_scoped_json(cJSON_CreateObject());

                for (int i = 0; i < c.builtin().attrs_size(); ++i) {
                    cJSON *item = cJSON_DeepCopy(data->GetObjectItem(c.builtin().attrs(i).c_str()));
                    if (!item) {
                        throw runtime_exc_t("Attempting to pick missing attribute.", backtrace.with(strprintf("attrs:%d", i)));
                    } else {
                        res->AddItemToObject(item->string, item);
                    }
                }
                return res;
            }
            break;
        case Builtin::MAPMERGE:
            {
                boost::shared_ptr<scoped_cJSON_t> left  = eval(c.args(0), env, backtrace.with("arg:0")),
                                                  right = eval(c.args(1), env, backtrace.with("arg:1"));
                if (left->type() != cJSON_Object) {
                    throw runtime_exc_t("Data must be an object", backtrace.with("arg:0"));
                }

                if (right->type() != cJSON_Object) {
                    throw runtime_exc_t("Data must be an object", backtrace.with("arg:1"));
                }

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
                boost::shared_ptr<scoped_cJSON_t> array  = eval(c.args(0), env, backtrace.with("arg:0"));
                if (array->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.", backtrace.with("arg:0"));
                }
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(array->DeepCopy()));
                res->AddItemToArray(eval(c.args(1), env, backtrace.with("arg:1"))->release());
                return res;
            }
            break;
        case Builtin::ARRAYCONCAT:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array1  = eval(c.args(0), env, backtrace.with("arg:0"));
                if (array1->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.", backtrace.with("arg:0"));
                }
                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> array2  = eval(c.args(1), env, backtrace.with("arg:1"));
                if (array2->type() != cJSON_Array) {
                    throw runtime_exc_t("The second argument must be an array.", backtrace.with("arg:1"));
                }
                // Create new array and deep copy all the elements
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));
                for(int i = 0; i < array1->GetArraySize(); i++) {
                    res->AddItemToArray(cJSON_DeepCopy(array1->GetArrayItem(i)));
                }
                for(int j = 0; j < array2->GetArraySize(); j++) {
                    res->AddItemToArray(cJSON_DeepCopy(array2->GetArrayItem(j)));
                }

                return res;
            }
            break;
        case Builtin::ARRAYSLICE:
            {
                int start, stop, length;
                bool start_unbounded = false, stop_unbounded = false;

                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array = eval(c.args(0), env, backtrace.with("arg:0"));
                if (array->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.", backtrace.with("arg:0"));
                }

                // Check second arg type
                {
                    boost::shared_ptr<scoped_cJSON_t> start_json  = eval(c.args(1), env, backtrace.with("arg:1"));
                    if (start_json->type() == cJSON_NULL) {
                        start_unbounded = true;
                    } else {
                        if (start_json->type() != cJSON_Number) {
                            throw runtime_exc_t("The second argument must be null or an integer.", backtrace.with("arg:1"));
                        }

                        float float_start = start_json->get()->valuedouble;
                        start = (int)float_start;
                        if (float_start != start) {
                            throw runtime_exc_t("The second argument must be null or an integer.", backtrace.with("arg:1"));
                        }
                    }
                }

                // Check third arg type
                {
                    boost::shared_ptr<scoped_cJSON_t> stop_json  = eval(c.args(2), env, backtrace.with("arg:2"));
                    if (stop_json->type() == cJSON_NULL) {
                        stop_unbounded = true;
                    } else {
                        if (stop_json->type() != cJSON_Number) {
                            throw runtime_exc_t("The third argument must be null or an integer.", backtrace.with("arg:2"));
                        }

                        float float_stop = stop_json->get()->valuedouble;
                        stop = (int)float_stop;
                        if (float_stop != stop) {
                            throw runtime_exc_t("The third argument must be null or an integer.", backtrace.with("arg:2"));
                        }
                    }
                }

                if (start_unbounded && stop_unbounded) {
                    return array;   // nothing to do
                }

                length = array->GetArraySize();

                if (start_unbounded) {
                    start = 0;
                }
                if (stop_unbounded) {
                    stop = length;
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
        case Builtin::ARRAYNTH:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array  = eval(c.args(0), env, backtrace.with("arg:0"));
                if (array->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.", backtrace.with("arg:0"));
                }

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> index_json  = eval(c.args(1), env, backtrace.with("arg:1"));
                if (index_json->type() != cJSON_Number) {
                    throw runtime_exc_t("The second argument must be an integer.", backtrace.with("arg:1"));
                }
                float float_index = index_json->get()->valuedouble;
                int index = (int)float_index;
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
            }
            break;
        case Builtin::ARRAYLENGTH:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array  = eval(c.args(0), env, backtrace.with("arg:0"));
                if (array->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.", backtrace.with("arg:0"));
                }
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(array->GetArraySize())));
            }
            break;
        case Builtin::ADD:
            {
                double result = 0.0;

                for (int i = 0; i < c.args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env, backtrace.with(strprintf("arg:%d", i)));
                    if (arg->type() != cJSON_Number) {
                        throw runtime_exc_t("All operands to ADD must be numbers.", backtrace.with(strprintf("arg:%d", i)));
                    }
                    result += arg->get()->valuedouble;
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(result)));
                return res;
            }
            break;
        case Builtin::SUBTRACT:
            {
                double result = 0.0;

                if (c.args_size() > 0) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(0), env, backtrace.with("arg:0"));
                    if (arg->type() != cJSON_Number) {
                        throw runtime_exc_t("All operands to SUBTRACT must be numbers.", backtrace.with("arg:0"));
                    }
                    if (c.args_size() == 1) {
                        result = -arg->get()->valuedouble;  // (- x) is negate
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c.args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env, backtrace.with(strprintf("arg:%d", i)));
                        if (arg->type() != cJSON_Number) {
                            throw runtime_exc_t("All operands to SUBTRACT must be numbers.", backtrace.with(strprintf("arg:%d", i)));
                        }
                        result -= arg->get()->valuedouble;
                    }
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(result)));
                return res;
            }
            break;
        case Builtin::MULTIPLY:
            {
                double result = 1.0;

                for (int i = 0; i < c.args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env, backtrace.with(strprintf("arg:%d", i)));
                    if (arg->type() != cJSON_Number) {
                        throw runtime_exc_t("All operands of MULTIPLY must be numbers.", backtrace.with(strprintf("arg:%d", i)));
                    }
                    result *= arg->get()->valuedouble;
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(result)));
                return res;
            }
            break;
        case Builtin::DIVIDE:
            {
                double result = 0.0;

                if (c.args_size() > 0) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(0), env, backtrace.with("arg:0"));
                    if (arg->type() != cJSON_Number) {
                        throw runtime_exc_t("All operands to DIVIDE must be numbers.", backtrace.with("arg:0"));
                    }
                    if (c.args_size() == 1) {
                        result = 1.0 / arg->get()->valuedouble;  // (/ x) is reciprocal
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c.args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env, backtrace.with(strprintf("arg:%d", i)));
                        if (arg->type() != cJSON_Number) {
                            throw runtime_exc_t("All operands to DIVIDE must be numbers.", backtrace.with(strprintf("arg:%d", i)));
                        }
                        result /= arg->get()->valuedouble;
                    }
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(result)));
                return res;
            }
            break;
        case Builtin::MODULO:
            {
                boost::shared_ptr<scoped_cJSON_t> lhs = eval(c.args(0), env, backtrace.with("arg:0")),
                    rhs = eval(c.args(1), env, backtrace.with("arg:1"));
                if (lhs->type() != cJSON_Number) {
                    throw runtime_exc_t("First operand of MOD must be a number.", backtrace.with("arg:0"));
                } else if (rhs->type() != cJSON_Number) {
                    throw runtime_exc_t("Second operand of MOD must be a number.", backtrace.with("arg:1"));
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(fmod(lhs->get()->valuedouble, rhs->get()->valuedouble))));
                return res;
            }
            break;
        case Builtin::COMPARE:
            {
                bool result = true;

                boost::shared_ptr<scoped_cJSON_t> lhs = eval(c.args(0), env, backtrace.with("arg:0"));

                for (int i = 1; i < c.args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> rhs = eval(c.args(i), env, backtrace.with(strprintf("arg:%d", i)));

                    int res = cJSON_cmp(lhs->get(), rhs->get(), backtrace.with(strprintf("arg:%d", i)));

                    switch (c.builtin().comparison()) {
                    case Builtin_Comparison_EQ:
                        result = (res == 0);
                        break;
                    case Builtin_Comparison_NE:
                        result = (res != 0);
                        break;
                    case Builtin_Comparison_LT:
                        result = (res < 0);
                        break;
                    case Builtin_Comparison_LE:
                        result = (res <= 0);
                        break;
                    case Builtin_Comparison_GT:
                        result = (res > 0);
                        break;
                    case Builtin_Comparison_GE:
                        result = (res >= 0);
                        break;
                    default:
                        crash("Unknown comparison operator.");
                        break;
                    if (!result)
                        break;
                    }

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
        case Builtin::LIMIT:
        case Builtin::ARRAYTOSTREAM:
        case Builtin::GROUPEDMAPREDUCE:
        case Builtin::UNION:
        case Builtin::RANGE:
            unreachable("eval called on a function that returns a stream (use eval_stream instead).");
            break;
        case Builtin::LENGTH:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));
                int length = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    ++length;
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(length)));
            }
            break;
        case Builtin::NTH:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> index_json  = eval(c.args(1), env, backtrace.with("arg:1"));
                if (index_json->type() != cJSON_Number) {
                    throw runtime_exc_t("The second argument must be an integer.", backtrace.with("arg:1"));
                }
                float index_float = index_json->get()->valuedouble;
                int index = (int)index_float;
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
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    res->AddItemToArray(json->DeepCopy());
                }

                return res;
            }
            break;
        case Builtin::REDUCE:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));

                throw runtime_exc_t("Not implemented reduce", backtrace);
                // Start off accumulator with the base
                //boost::shared_ptr<scoped_cJSON_t> acc = eval(c.builtin().reduce().base(), env);

                //for (json_stream_t::iterator it  = stream.begin();
                //                             it != stream.end();
                //                             ++it)
                //{
                //    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                //    env->scope.put_in_scope(c.builtin().reduce().var1(), acc);
                //    env->scope.put_in_scope(c.builtin().reduce().var2(), *it);

                //    acc = eval(c.builtin().reduce().body(), env);
                //}
                //return acc;
            }
            break;
        case Builtin::ALL:
            {
                bool result = true;

                for (int i = 0; i < c.args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env, backtrace.with(strprintf("arg:%d", i)));
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

                for (int i = 0; i < c.args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env, backtrace.with(strprintf("arg:%d", i)));
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

class predicate_t {
public:
    predicate_t(const Predicate &_pred, runtime_environment_t _env, const backtrace_t &_backtrace)
        : pred(_pred), env(_env), backtrace(_backtrace)
    { }
    bool operator()(boost::shared_ptr<scoped_cJSON_t> json) {
        variable_val_scope_t::new_scope_t scope_maker(&env.scope);
        env.scope.put_in_scope(pred.arg(), json);
        implicit_value_setter_t impliciter(&env.implicit_attribute_value, json);
        boost::shared_ptr<scoped_cJSON_t> a_bool = eval(pred.body(), &env, backtrace.with("predicate"));

        if (a_bool->type() == cJSON_True) {
            return true;
        } else if (a_bool->type() == cJSON_False) {
            return false;
        } else {
            throw runtime_exc_t("Predicate failed to evaluate to a bool", backtrace.with("predicate"));
        }
    }
private:
    Predicate pred;
    runtime_environment_t env;
    backtrace_t backtrace;
};

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

boost::shared_ptr<scoped_cJSON_t> map(std::string arg, const Term &term, runtime_environment_t env, boost::shared_ptr<scoped_cJSON_t> val, const backtrace_t &backtrace) {
    variable_val_scope_t::new_scope_t scope_maker(&env.scope);
    env.scope.put_in_scope(arg, val);
    implicit_value_setter_t impliciter(&env.implicit_attribute_value, val);
    return eval(term, &env, backtrace);
}

boost::shared_ptr<json_stream_t> concatmap(std::string arg, const Term &term, runtime_environment_t env, boost::shared_ptr<scoped_cJSON_t> val, const backtrace_t &backtrace) {
    variable_val_scope_t::new_scope_t scope_maker(&env.scope);
    env.scope.put_in_scope(arg, val);
    implicit_value_setter_t impliciter(&env.implicit_attribute_value, val);
    return eval_stream(term, &env, backtrace);
}

boost::shared_ptr<json_stream_t> eval_stream(const Term::Call &c, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (c.builtin().type()) {
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
        case Builtin::ARRAYCONCAT:
        case Builtin::ARRAYSLICE:
        case Builtin::ARRAYNTH:
        case Builtin::ARRAYLENGTH:
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
                shared_scoped_less comparator(backtrace);
                std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less> groups(comparator);
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    boost::shared_ptr<scoped_cJSON_t> group_mapped_row, value_mapped_row, reduced_row;

                    // Figure out which group we belong to
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                        env->scope.put_in_scope(c.builtin().grouped_map_reduce().group_mapping().arg(), json);
                        implicit_value_setter_t impliciter(&env->implicit_attribute_value, json);
                        group_mapped_row = eval(c.builtin().grouped_map_reduce().group_mapping().body(), env, backtrace.with("group_mapping"));
                    }

                    // Map the value for comfy reduction goodness
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                        env->scope.put_in_scope(c.builtin().grouped_map_reduce().value_mapping().arg(), json);
                        implicit_value_setter_t impliciter(&env->implicit_attribute_value, json);
                        value_mapped_row = eval(c.builtin().grouped_map_reduce().value_mapping().body(), env, backtrace.with("value_mapping"));
                    }

                    // Do the reduction
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                        std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less>::iterator
                            elem = groups.find(group_mapped_row);
                        if(elem != groups.end()) {
                            env->scope.put_in_scope(c.builtin().grouped_map_reduce().reduction().var1(),
                                                    (*elem).second);
                        } else {
                            env->scope.put_in_scope(c.builtin().grouped_map_reduce().reduction().var1(),
                                                    eval(c.builtin().grouped_map_reduce().reduction().base(), env, backtrace.with("reduction").with("base")));
                        }
                        env->scope.put_in_scope(c.builtin().grouped_map_reduce().reduction().var2(), value_mapped_row);

                        reduced_row = eval(c.builtin().grouped_map_reduce().reduction().body(), env, backtrace.with("reduction").with("body"));
                    }

                    // Phew, update the relevant group
                    groups[group_mapped_row] = reduced_row;

                }

                // Phew, convert the whole shebang to a stream
                // TODO this is an extra copy we just need a gracefull way to
                // deal with this
                std::list<boost::shared_ptr<scoped_cJSON_t> > res;

                std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less>::iterator it;
                for(it = groups.begin(); it != groups.end(); ++it) {
                    res.push_back((*it).second);
                }

                return boost::shared_ptr<in_memory_stream_t>(new in_memory_stream_t(res.begin(), res.end()));
            }
            break;
        case Builtin::FILTER:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));
                predicate_t p(c.builtin().filter().predicate(), *env, backtrace);
                return boost::shared_ptr<json_stream_t>(new filter_stream_t<predicate_t>(stream, p));
            }
            break;
        case Builtin::MAP:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));

                return boost::shared_ptr<json_stream_t>(new mapping_stream_t<boost::function<boost::shared_ptr<scoped_cJSON_t>(boost::shared_ptr<scoped_cJSON_t>)> >(
                                                                stream, boost::bind(&map, c.builtin().map().mapping().arg(), c.builtin().map().mapping().body(), *env, _1, backtrace.with("mapping"))));
            }
            break;
        case Builtin::CONCATMAP:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));

                return boost::shared_ptr<json_stream_t>(new concat_mapping_stream_t<boost::function<boost::shared_ptr<json_stream_t>(boost::shared_ptr<scoped_cJSON_t>)> >(
                                                                stream, boost::bind(&concatmap, c.builtin().concat_map().mapping().arg(), c.builtin().concat_map().mapping().body(), *env, _1, backtrace.with("mapping"))));
            }
            break;
        case Builtin::ORDERBY:
            {
                ordering_t o(c.builtin().order_by(), backtrace.with("order_by"));
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));

                boost::shared_ptr<in_memory_stream_t> sorted_stream(new in_memory_stream_t(stream));
                sorted_stream->sort(o);
                return sorted_stream;
            }
            break;
        case Builtin::DISTINCT:
            {
                shared_scoped_less comparator(backtrace);
                std::set<boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less> seen(comparator);

                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    seen.insert(json);
                }

                return boost::shared_ptr<in_memory_stream_t>(new in_memory_stream_t(seen.begin(), seen.end()));
            }
            break;
        case Builtin::LIMIT:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env, backtrace.with("arg:0"));

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> limit_json  = eval(c.args(1), env, backtrace.with("arg:1"));
                if (limit_json->type() != cJSON_Number) {
                    throw runtime_exc_t("The limit must be a nonnegative integer.", backtrace.with("arg:1"));
                }
                float limit_float = limit_json->get()->valuedouble;
                int limit = (int)limit_float;
                if (limit_float != limit || limit < 0) {
                    throw runtime_exc_t("The limit must be a nonnegative integer.", backtrace.with("arg:1"));
                }

                return boost::shared_ptr<json_stream_t>(new limit_stream_t(stream, limit));
            }
            break;
        case Builtin::UNION:
            {
                union_stream_t::stream_list_t streams;

                streams.push_back(eval_stream(c.args(0), env, backtrace.with("arg:0")));

                streams.push_back(eval_stream(c.args(1), env, backtrace.with("arg:1")));

                return boost::shared_ptr<json_stream_t>(new union_stream_t(streams));
            }
            break;
        case Builtin::ARRAYTOSTREAM:
            {
                boost::shared_ptr<scoped_cJSON_t> array = eval(c.args(0), env, backtrace.with("arg:0"));
                json_array_iterator_t it(array->get());

                return boost::shared_ptr<json_stream_t>(new in_memory_stream_t(it));
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

namespace_repo_t<rdb_protocol_t>::access_t eval(const TableRef &t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    boost::optional<std::pair<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<rdb_protocol_t> > > > namespace_info =
        env->semilattice_metadata->get().rdb_namespaces.get_namespace_by_name(t.table_name());

    if (namespace_info) {
        return namespace_repo_t<rdb_protocol_t>::access_t(env->ns_repo, namespace_info->first, env->interruptor);
    } else {
        throw runtime_exc_t(strprintf("Namespace %s either not found, ambigious or namespace metadata in conflict.", t.table_name().c_str()), backtrace);
    }
}

view_t eval_view(const Term::Call &c, UNUSED runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (c.builtin().type()) {
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
        case Builtin::ARRAYCONCAT:
        case Builtin::ARRAYSLICE:
        case Builtin::ARRAYNTH:
        case Builtin::ARRAYLENGTH:
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
                view_t view = eval_view(c.args(0), env, backtrace.with("arg:0"));
                predicate_t p(c.builtin().filter().predicate(), *env, backtrace);
                return view_t(view.access, boost::shared_ptr<json_stream_t>(new filter_stream_t<predicate_t>(view.stream, p)));
            }
            break;
        case Builtin::ORDERBY:
            {
                ordering_t o(c.builtin().order_by(), backtrace.with("order_by"));
                view_t view = eval_view(c.args(0), env, backtrace.with("arg:0"));

                boost::shared_ptr<in_memory_stream_t> sorted_stream(new in_memory_stream_t(view.stream));
                sorted_stream->sort(o);
                return view_t(view.access, sorted_stream);
            }
            break;
        case Builtin::LIMIT:
            {
                view_t pview = eval_view(c.args(0), env, backtrace.with("arg:0"));

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> limit_json  = eval(c.args(1), env, backtrace.with("arg:1"));
                if (limit_json->type() != cJSON_Number) {
                    throw runtime_exc_t("The limit must be a nonnegative integer.", backtrace.with("arg:1"));
                }
                float limit_float = limit_json->get()->valuedouble;
                int limit = (int)limit_float;
                if (limit_float != limit || limit < 0) {
                    throw runtime_exc_t("The limit must be a nonnegative integer.", backtrace.with("arg:1"));
                }

                return view_t(pview.access, boost::shared_ptr<json_stream_t>(new limit_stream_t(pview.stream, limit)));
            }
            throw runtime_exc_t("Unimplemented: Builtin::LIMIT", backtrace);
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

view_t eval_view(const Term &t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    switch (t.type()) {
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
            return eval_view(t.call(), env, backtrace);
            break;
        case Term::IF:
            {
                boost::shared_ptr<scoped_cJSON_t> test = eval(t.if_().test(), env, backtrace.with("test"));
                if (test->type() != cJSON_True && test->type() != cJSON_False) {
                    throw runtime_exc_t("The IF test must evaluate to a boolean.", backtrace.with("test"));
                }

                if (test->type() == cJSON_True) {
                    return eval_view(t.if_().true_branch(), env, backtrace.with("true"));
                } else {
                    return eval_view(t.if_().false_branch(), env, backtrace.with("false"));
                }
            }
            break;
        case Term::GETBYKEY:
            crash("unimplemented");
            break;
        case Term::TABLE:
            return eval_view(t.table(), env, backtrace.with("table_ref"));
            break;
        default:
            unreachable();
    }
    unreachable();
}

view_t eval_view(const Term::Table &t, runtime_environment_t *env, const backtrace_t &backtrace) THROWS_ONLY(runtime_exc_t) {
    namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(t.table_ref(), env, backtrace);
    boost::shared_ptr<json_stream_t> stream(new batched_rget_stream_t(ns_access, env->interruptor, key_range_t::universe(), 100, backtrace));
    return view_t(ns_access, stream);
}

} //namespace query_language
