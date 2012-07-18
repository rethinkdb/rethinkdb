#include "rdb_protocol/query_language.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <math.h>

#include "http/json.hpp"

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

function_type_t::function_type_t(const term_type_t& _arg_type, int _n_args, const term_type_t& _return_type)
    : n_args(_n_args), return_type(_return_type) {
    arg_type[0] = _arg_type;
    /* This weird loop starting from 1 is so that we don't fill any more slots
    if `n_args` is -1. */
    for (int i = 1; i < _n_args; ++i) {
        arg_type[i] = _arg_type;
    }
}

function_type_t::function_type_t(const term_type_t& _arg1_type, const term_type_t& _arg2_type, const term_type_t& _return_type)
    : n_args(2), return_type(_return_type) {
    arg_type[0] = _arg1_type;
    arg_type[1] = _arg2_type;
}

const term_type_t& function_type_t::get_arg_type(int n) const {
    rassert(n >= 0);
    if (is_variadic()) {
        return arg_type[0];
    } else {
        rassert(n < n_args);
        return arg_type[n];
    }
}

const term_type_t& function_type_t::get_return_type() const {
    return return_type;
}

bool function_type_t::is_variadic() const {
    return n_args == -1;
}

int function_type_t::get_n_args() const {
    rassert(!is_variadic());
    return n_args;
}

term_type_t get_term_type(const Term &t, variable_type_scope_t *scope) {
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    t.GetReflection()->ListFields(t, &fields);
    int field_count = fields.size();

    check_protobuf(field_count <= 2);

    switch (t.type()) {
    case Term::VAR:
        check_protobuf(t.has_var());
        if (!scope->is_in_scope(t.var())) {
            throw bad_query_exc_t(strprintf("symbol '%s' is not in scope", t.var().c_str()));
        }
        return scope->get(t.var());
        break;
    case Term::LET:
        {
            check_protobuf(t.has_let());
            scope->push(); //create a new scope
            for (int i = 0; i < t.let().binds_size(); ++i) {
                scope->put_in_scope(t.let().binds(i).var(), get_term_type(t.let().binds(i).term(), scope));
            }
            term_type_t res = get_term_type(t.let().expr(), scope);
            scope->pop();
            return res;
        }
        break;
    case Term::CALL:
        {
            check_protobuf(t.has_call());
            function_type_t signature = get_function_type(t.call().builtin(), scope);
            if (!signature.is_variadic()) {
                int n_args = signature.get_n_args();
                if (t.call().args_size() != n_args) {
                    const char* fn_name = Builtin::BuiltinType_Name(t.call().builtin().type()).c_str();
                    throw bad_query_exc_t(strprintf(
                        "%s takes %d argument%s (%d given)",
                        fn_name,
                        n_args,
                        n_args > 1 ? "s" : "",
                        t.call().args_size()
                        ));
                }
            }
            for (int i = 0; i < t.call().args_size(); ++i) {
                check_term_type(t.call().args(i), signature.get_arg_type(i), scope);
            }
            return signature.get_return_type();
        }
        break;
    case Term::IF:
        {
            check_protobuf(t.has_if_());
            check_term_type(t.if_().test(), TERM_TYPE_JSON, scope);

            term_type_t true_branch = get_term_type(t.if_().true_branch(), scope);
            term_type_t false_branch = get_term_type(t.if_().false_branch(), scope);

            term_type_t combined_type;
            if (!term_type_least_upper_bound(true_branch, false_branch, &combined_type)) {
                throw bad_query_exc_t(strprintf(
                    "true-branch has type %s, but false-branch has type %s",
                    term_type_name(true_branch).c_str(),
                    term_type_name(false_branch).c_str()
                    ));
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
            check_term_type(t.array(i), TERM_TYPE_JSON, scope);
        }
        return TERM_TYPE_JSON;
        break;
    case Term::OBJECT:
        if (t.object_size() == 0) { // empty objects are valid
            check_protobuf(field_count == 1);
        }
        for (int i = 0; i < t.object_size(); ++i) {
            check_term_type(t.object(i).term(), TERM_TYPE_JSON, scope);
        }
        return TERM_TYPE_JSON;
        break;
    case Term::GETBYKEY:
        check_protobuf(t.has_get_by_key());
        check_term_type(t.get_by_key().key(), TERM_TYPE_JSON, scope);
        return TERM_TYPE_JSON;
        break;
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

void check_term_type(const Term &t, term_type_t expected, variable_type_scope_t *scope) {
    term_type_t actual = get_term_type(t, scope);
    if (!term_type_is_convertible(actual, expected)) {
        throw bad_query_exc_t(strprintf("expected a %s; got a %s",
            term_type_name(expected).c_str(), term_type_name(actual).c_str()));
    }
}

function_type_t get_function_type(const Builtin &b, variable_type_scope_t *scope) {
    std::vector<const google::protobuf::FieldDescriptor *> fields;

    b.GetReflection()->ListFields(b, &fields);

    int field_count = fields.size();

    check_protobuf(field_count <= 2);

    // this is a bit cleaner when we check well-formedness separate
    // from returning the type

    switch (b.type()) {
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
    case Builtin::HASATTR:
        check_protobuf(b.has_attr());
        break;
    case Builtin::PICKATTRS:
        check_protobuf(b.attrs_size());
        break;
    case Builtin::FILTER:
        check_protobuf(b.has_filter());
        check_predicate_type(b.filter().predicate(), scope);
        break;
    case Builtin::MAP:
        check_protobuf(b.has_map());
        check_mapping_type(b.map().mapping(), TERM_TYPE_JSON, scope);
        break;
    case Builtin::CONCATMAP:
        check_protobuf(b.has_concat_map());
        check_mapping_type(b.map().mapping(), TERM_TYPE_STREAM, scope);
        break;
    case Builtin::ORDERBY:
        check_protobuf(b.order_by_size() > 0);
        break;
    case Builtin::REDUCE:
        check_protobuf(b.has_reduce());
        check_reduction_type(b.reduce(), scope);
        break;
    case Builtin::GROUPEDMAPREDUCE:
        check_protobuf(b.has_grouped_map_reduce());
        check_mapping_type(b.grouped_map_reduce().group_mapping(), TERM_TYPE_JSON, scope);
        check_mapping_type(b.grouped_map_reduce().value_mapping(), TERM_TYPE_JSON, scope);
        check_reduction_type(b.grouped_map_reduce().reduction(), scope);
        break;
    case Builtin::RANGE:
        check_protobuf(b.has_range());
        if (b.range().has_lowerbound()) {
            check_term_type(b.range().lowerbound(), TERM_TYPE_JSON, scope);
        }
        if (b.range().has_upperbound()) {
            check_term_type(b.range().upperbound(), TERM_TYPE_JSON, scope);
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
            return function_type_t(TERM_TYPE_JSON, 1, TERM_TYPE_JSON);
            break;
        case Builtin::MAPMERGE:
        case Builtin::ARRAYAPPEND:
        case Builtin::ARRAYCONCAT:
        case Builtin::ARRAYNTH:
        case Builtin::MODULO:
            return function_type_t(TERM_TYPE_JSON, 2, TERM_TYPE_JSON);
            break;
        case Builtin::ADD:
        case Builtin::SUBTRACT:
        case Builtin::MULTIPLY:
        case Builtin::DIVIDE:
        case Builtin::COMPARE:
        case Builtin::ANY:
        case Builtin::ALL:
            return function_type_t(TERM_TYPE_JSON, -1, TERM_TYPE_JSON);  // variadic JSON type
            break;
        case Builtin::ARRAYSLICE:
            return function_type_t(TERM_TYPE_JSON, 3, TERM_TYPE_JSON);
            break;
        case Builtin::FILTER:
        case Builtin::MAP:
        case Builtin::CONCATMAP:
        case Builtin::ORDERBY:
        case Builtin::DISTINCT:
            return function_type_t(TERM_TYPE_STREAM, 1, TERM_TYPE_STREAM);
            break;
        case Builtin::LIMIT:
            return function_type_t(TERM_TYPE_STREAM, TERM_TYPE_JSON, TERM_TYPE_STREAM);
            break;
        case Builtin::NTH:
            return function_type_t(TERM_TYPE_STREAM, TERM_TYPE_JSON, TERM_TYPE_JSON);
            break;
        case Builtin::LENGTH:
        case Builtin::STREAMTOARRAY:
        case Builtin::REDUCE:
        case Builtin::GROUPEDMAPREDUCE:
            return function_type_t(TERM_TYPE_STREAM, 1, TERM_TYPE_JSON);
            break;
        case Builtin::UNION:
            return function_type_t(TERM_TYPE_STREAM, 2, TERM_TYPE_STREAM);
            break;
        case Builtin::ARRAYTOSTREAM:
            return function_type_t(TERM_TYPE_JSON, 1, TERM_TYPE_STREAM);
            break;
        case Builtin::RANGE:
            return function_type_t(TERM_TYPE_VIEW, 1, TERM_TYPE_VIEW);
            break;
        default:
            crash("unreachable");
            break;
    }

    crash("unreachable");
}

void check_reduction_type(const Reduction &r, variable_type_scope_t *scope) {
    check_term_type(r.base(), TERM_TYPE_JSON, scope);

    new_scope_t scope_maker(scope);
    scope->put_in_scope(r.var1(), TERM_TYPE_JSON);
    scope->put_in_scope(r.var2(), TERM_TYPE_JSON);

    check_term_type(r.body(), TERM_TYPE_JSON, scope);
}

void check_mapping_type(const Mapping &m, term_type_t return_type, variable_type_scope_t *scope) {
    new_scope_t scope_maker(scope);
    scope->put_in_scope(m.arg(), TERM_TYPE_JSON);

    check_term_type(m.body(), return_type, scope);
}

void check_predicate_type(const Predicate &p, variable_type_scope_t *scope) {
    new_scope_t scope_maker(scope);
    scope->put_in_scope(p.arg(), TERM_TYPE_JSON);

    check_term_type(p.body(), TERM_TYPE_JSON, scope);
}

void check_read_query_type(const ReadQuery &rq, variable_type_scope_t *scope) {
    /* Read queries could return anything--a view, a stream, a JSON, or an
    error. Views will be automatically converted to streams at evaluation time.
    */
    get_term_type(rq.term(), scope);
}

void check_write_query_type(const WriteQuery &w, variable_type_scope_t *scope) {
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    w.GetReflection()->ListFields(w, &fields);
    check_protobuf(fields.size() == 2);

    switch (w.type()) {
        case WriteQuery::UPDATE:
            check_protobuf(w.has_update());
            check_term_type(w.update().view(), TERM_TYPE_VIEW, scope);
            check_mapping_type(w.update().mapping(), TERM_TYPE_JSON, scope);
            break;
        case WriteQuery::DELETE:
            check_protobuf(w.has_delete_());
            check_term_type(w.delete_().view(), TERM_TYPE_VIEW, scope);
            break;
        case WriteQuery::MUTATE:
            check_protobuf(w.has_mutate());
            check_term_type(w.mutate().view(), TERM_TYPE_VIEW, scope);
            check_mapping_type(w.mutate().mapping(), TERM_TYPE_JSON, scope);
            break;
        case WriteQuery::INSERT:
            check_protobuf(w.has_insert());
            for (int i = 0; i < w.insert().terms_size(); ++i) {
                check_term_type(w.insert().terms(i), TERM_TYPE_JSON, scope);
            }
            break;
        case WriteQuery::INSERTSTREAM:
            check_protobuf(w.has_insert_stream());
            check_term_type(w.insert_stream().stream(), TERM_TYPE_STREAM, scope);
            break;
        case WriteQuery::FOREACH:
            {
                check_protobuf(w.has_for_each());
                check_term_type(w.for_each().stream(), TERM_TYPE_STREAM, scope);

                new_scope_t scope_maker(scope);
                scope->put_in_scope(w.for_each().var(), TERM_TYPE_JSON);
                for (int i = 0; i < w.for_each().queries_size(); ++i) {
                    check_write_query_type(w.for_each().queries(i), scope);
                }
            }
            break;
        case WriteQuery::POINTUPDATE:
            check_protobuf(w.has_point_update());
            check_term_type(w.point_update().key(), TERM_TYPE_JSON, scope);
            check_mapping_type(w.point_update().mapping(), TERM_TYPE_JSON, scope);
            break;
        case WriteQuery::POINTDELETE:
            check_protobuf(w.has_point_delete());
            check_term_type(w.point_delete().key(), TERM_TYPE_JSON, scope);
            break;
        case WriteQuery::POINTMUTATE:
            check_protobuf(w.has_point_mutate());
            check_term_type(w.point_mutate().key(), TERM_TYPE_JSON, scope);
            check_mapping_type(w.point_mutate().mapping(), TERM_TYPE_JSON, scope);
            break;
        default:
            unreachable("unhandled WriteQuery");
    }
}

void check_query_type(const Query &q, variable_type_scope_t *scope) {
    switch (q.type()) {
    case Query::READ:
        check_protobuf(q.has_read_query());
        check_protobuf(!q.has_write_query());
        check_read_query_type(q.read_query(), scope);
        break;
    case Query::WRITE:
        check_protobuf(q.has_write_query());
        check_protobuf(!q.has_read_query());
        check_write_query_type(q.write_query(), scope);
        break;
    default:
        unreachable("unhandled Query");
    }
}

int cJSON_cmp(cJSON *l, cJSON *r) {
    switch (l->type) {
        case cJSON_False:
            if (r->type == cJSON_True) {
                return -1;
            } else if (r->type == cJSON_False) {
                return 0;
            } else {
                throw runtime_exc_t("Booleans can only be compared to other booleans");
            }
            break;
        case cJSON_True:
            if (r->type == cJSON_True) {
                return 0;
            } else if (r->type == cJSON_False) {
                return 1;
            } else {
                throw runtime_exc_t("Booleans can only be compared to other booleans");
            }
            break;
        case cJSON_NULL:
            throw runtime_exc_t("Can't compare null to anything");
            break;
        case cJSON_Number:
            if (r->type != cJSON_Number) {
                throw runtime_exc_t("Numbers can only be compared to other numbers.");
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
                throw runtime_exc_t("Strings can only be compared to other strings.");
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
                    int cmp = cJSON_cmp(cJSON_GetArrayItem(l, i), cJSON_GetArrayItem(r, i));
                    if (cmp) {
                        return cmp;
                    }
                }
                return -1;  // e.g. cmp([0], [0, 1]);
            } else {
                throw runtime_exc_t("Strings can only be compared to other strings.");
            }
            break;
        case cJSON_Object:
            throw runtime_exc_t("Can't compare objects.");
            break;
        default:
            unreachable();
            break;
    }
}

struct shared_scoped_less {
    bool operator()(const boost::shared_ptr<scoped_cJSON_t> &a,
                      const boost::shared_ptr<scoped_cJSON_t> &b) {
        if (a->type() == b->type()) {
            return cJSON_cmp(a->get(), b->get()) < 0;
        } else {
            return a->type() > b->type();
        }
    }
};

void execute(const Query &q, runtime_environment_t *env, Response *res) {
    if (q.type() == Query::READ) {
        execute(q.read_query(), env, res);
    } else if (q.type() == Query::WRITE) {
        execute(q.write_query(), env, res);
    } else {
        crash("unreachable");
    }
}

void execute(const ReadQuery &r, runtime_environment_t *env, Response *res) THROWS_ONLY(runtime_exc_t) {
    term_type_t type = get_term_type(r.term(), &env->type_scope);

    switch (type) {
    case TERM_TYPE_JSON: {
        boost::shared_ptr<scoped_cJSON_t> json = eval(r.term(), env);
        res->add_response(json->PrintUnformatted());
        break;
    }
    case TERM_TYPE_STREAM:
    case TERM_TYPE_VIEW: {
        boost::shared_ptr<json_stream_t> stream = eval_stream(r.term(), env);
        while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            res->add_response(json->PrintUnformatted());
        }
        break;
    }
    case TERM_TYPE_ARBITRARY: {
        eval(r.term(), env);
        unreachable("This term has type `TERM_TYPE_ARBITRARY`, so evaluating "
            "it should throw `runtime_exc_t`.");
    }
    default:
        unreachable("read query type invalid.");
    }
}

void insert(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> data, runtime_environment_t *env) {
    if (!data->GetObjectItem("id")) {
        throw runtime_exc_t("Must have a field named id.");
    }

    rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t(cJSON_print_std_string(data->GetObjectItem("id"))), data));
    ns_access.get_namespace_if()->write(write, order_token_t::ignore, &env->interruptor);
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, cJSON *id, runtime_environment_t *env) {
    rdb_protocol_t::write_t write(rdb_protocol_t::point_delete_t(store_key_t(cJSON_print_std_string(id))));
    ns_access.get_namespace_if()->write(write, order_token_t::ignore, &env->interruptor);
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> id, runtime_environment_t *env) {
    point_delete(ns_access, id->get(), env);
}

void execute(const WriteQuery &w, runtime_environment_t *env, Response *res) THROWS_ONLY(runtime_exc_t) {
    switch (w.type()) {
        case WriteQuery::UPDATE:
            {
                view_t view = eval_view(w.update().view().table(), env);

                int updated = 0,
                    error = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope);

                    env->scope.put_in_scope(w.update().mapping().arg(), json);
                    boost::shared_ptr<scoped_cJSON_t> val = eval(w.update().mapping().body(), env);
                    if (!cJSON_Equal(json->GetObjectItem("id"),
                                     val->GetObjectItem("id"))) {
                        error++;
                    } else {
                        insert(view.access, val, env);
                        updated++;
                    }
                }

                res->add_response(strprintf("{\"updated\": %d, \"errors\": %d}", updated, error));
            }
            break;
        case WriteQuery::DELETE:
            {
                view_t view = eval_view(w.delete_().view().table(), env);

                int deleted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    point_delete(view.access, json->GetObjectItem("id"), env);
                    deleted++;
                }

                res->add_response(strprintf("{\"deleted\": %d}", deleted));
            }
            break;
        case WriteQuery::MUTATE:
            {
                view_t view = eval_view(w.update().view().table(), env);

                int modified = 0, deleted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                    env->scope.put_in_scope(w.update().mapping().arg(), json);
                    boost::shared_ptr<scoped_cJSON_t> val = eval(w.update().mapping().body(), env);

                    if (val->type() == cJSON_NULL) {
                        point_delete(view.access, json->GetObjectItem("id"), env);
                        ++deleted;
                    } else {
                        insert(view.access, eval(w.update().mapping().body(), env), env);
                        ++modified;
                    }
                }

                res->add_response(strprintf("{\"modified\": %d, \"deleted\": %d}", modified, deleted));
            }
            break;
        case WriteQuery::INSERT:
            {
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.insert().table_ref(), env);
                for (int i = 0; i < w.insert().terms_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> data = eval(w.insert().terms(i), env);

                    insert(ns_access, data, env);
                }

                res->add_response(strprintf("{\"inserted\": %d}", w.insert().terms_size()));
            }
            break;
        case WriteQuery::INSERTSTREAM:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(w.insert_stream().stream(), env);

                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.insert_stream().table_ref(), env);

                int inserted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    inserted++;
                    insert(ns_access, json, env);
                }

                res->add_response(strprintf("{\"inserted\": %d}", inserted));
            }
            break;
        case WriteQuery::FOREACH:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(w.for_each().stream(), env);

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                    env->scope.put_in_scope(w.for_each().var(), json);

                    for (int i = 0; i < w.for_each().queries_size(); ++i) {
                        execute(w.for_each().queries(i), env, res);
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

                boost::shared_ptr<scoped_cJSON_t> original_val = eval(get, env);
                new_val_scope_t scope_maker(&env->scope);
                env->scope.put_in_scope(w.point_update().mapping().arg(), original_val);

                boost::shared_ptr<scoped_cJSON_t> new_val = eval(w.point_update().mapping().body(), env);

                /* Now we insert the new value. */
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.point_update().table_ref(), env);

                insert(ns_access, new_val, env);

                res->add_response(strprintf("{\"updated\": %d, \"errors\": %d}", 1, 0));
            }
            break;
        case WriteQuery::POINTDELETE:
            {
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.point_delete().table_ref(), env);
                boost::shared_ptr<scoped_cJSON_t> id = eval(w.point_delete().key(), env);
                point_delete(ns_access, id, env);

                res->add_response(strprintf("{\"deleted\": %d}", 1));
            }
            break;
        case WriteQuery::POINTMUTATE:
            throw runtime_exc_t("Unimplemented: POINTMUTATE");
            break;
        default:
            unreachable();
    }
}

//This doesn't create a scope for the evals
void eval_let_binds(const Term::Let &let, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    // Go through the bindings in a let and add them one by one
    for (int i = 0; i < let.binds_size(); ++i) {
        term_type_t type = get_term_type(let.binds(i).term(), &env->type_scope);

        if (type == TERM_TYPE_JSON) {
            env->scope.put_in_scope(let.binds(i).var(),
                    eval(let.binds(i).term(), env));
        } else if (type == TERM_TYPE_STREAM || type == TERM_TYPE_VIEW) {
            env->stream_scope.put_in_scope(let.binds(i).var(),
                    boost::shared_ptr<stream_multiplexer_t>(new stream_multiplexer_t(eval_stream(let.binds(i).term(), env))));
        } else if (type == TERM_TYPE_ARBITRARY) {
            eval(let.binds(i).term(), env);
            unreachable("This term has type `TERM_TYPE_ARBITRARY`, so "
                "evaluating it must throw  `runtime_exc_t`.");
        }

        env->type_scope.put_in_scope(let.binds(i).var(),
                                     type);
    }
}

boost::shared_ptr<scoped_cJSON_t> eval(const Term &t, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    switch (t.type()) {
        case Term::VAR:
            return env->scope.get(t.var());
            break;
        case Term::LET:
            {
                // Push the scope
                variable_val_scope_t::new_scope_t new_scope(&env->scope);
                variable_stream_scope_t::new_scope_t new_stream_scope(&env->stream_scope);
                variable_type_scope_t::new_scope_t new_type_scope(&env->type_scope);

                eval_let_binds(t.let(), env);

                return eval(t.let().expr(), env);
            }
            break;
        case Term::CALL:
            return eval(t.call(), env);
            break;
        case Term::IF:
            {
                boost::shared_ptr<scoped_cJSON_t> test = eval(t.if_().test(), env);
                if (test->type() != cJSON_True && test->type() != cJSON_False) {
                    throw runtime_exc_t("The IF test must evaluate to a boolean.");
                }

                boost::shared_ptr<scoped_cJSON_t> res;
                if (test->type() == cJSON_True) {
                    res = eval(t.if_().true_branch(), env);
                } else {
                    res = eval(t.if_().false_branch(), env);
                }
                return res;
            }
            break;
        case Term::ERROR:
            throw runtime_exc_t(t.error());
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
                    res->AddItemToArray(eval(t.array(i), env)->release());
                }
                return res;
            }
            break;
        case Term::OBJECT:
            {
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateObject()));
                for (int i = 0; i < t.object_size(); ++i) {
                    std::string item_name(t.object(i).var());
                    res->AddItemToObject(item_name.c_str(), eval(t.object(i).term(), env)->release());
                }
                return res;
            }
            break;
        case Term::GETBYKEY:
            {
                boost::optional<std::pair<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<rdb_protocol_t> > > > namespace_info =
                    env->semilattice_metadata->get().rdb_namespaces.get_namespace_by_name(t.get_by_key().table_ref().table_name());

                if (!namespace_info) {
                    throw runtime_exc_t(strprintf("Namespace %s either not found, ambigious or namespace metadata in conflict.", t.get_by_key().table_ref().table_name().c_str()));
                }

                if (t.get_by_key().attrname() != "id") {
                    throw runtime_exc_t(strprintf("Attribute: %s is not the primary key and thus cannot be selected upon.", t.get_by_key().attrname().c_str()));
                }

                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(t.get_by_key().table_ref(), env);

                boost::shared_ptr<scoped_cJSON_t> key = eval(t.get_by_key().key(), env);
                rdb_protocol_t::read_t read(rdb_protocol_t::point_read_t(store_key_t(key->Print())));
                rdb_protocol_t::read_response_t res = ns_access.get_namespace_if()->read(read, order_token_t::ignore, &env->interruptor);

                rdb_protocol_t::point_read_response_t *p_res = boost::get<rdb_protocol_t::point_read_response_t>(&res.response);
                return p_res->data;
                break;
            }
        case Term::TABLE:
            crash("Term::TABLE must be evaluated with eval_stream or eval_view");

        case Term::JAVASCRIPT:
            // TODO (rntz)
            crash("unimplemented");

        default:
            unreachable();
    }
    unreachable();
}

boost::shared_ptr<json_stream_t> eval_stream(const Term &t, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    switch (t.type()) {
        case Term::VAR:
            return boost::shared_ptr<json_stream_t>(new stream_multiplexer_t::stream_t(env->stream_scope.get(t.var())));
            break;
        case Term::LET:
            {
                // Push the scope
                variable_val_scope_t::new_scope_t new_scope(&env->scope);
                variable_stream_scope_t::new_scope_t new_stream_scope(&env->stream_scope);
                variable_type_scope_t::new_scope_t new_type_scope(&env->type_scope);

                eval_let_binds(t.let(), env);

                return eval_stream(t.let().expr(), env);
            }
            break;
        case Term::CALL:
            return eval_stream(t.call(), env);
            break;
        case Term::IF:
            {
                boost::shared_ptr<scoped_cJSON_t> test = eval(t.if_().test(), env);
                if (test->type() != cJSON_True && test->type() != cJSON_False) {
                    throw runtime_exc_t("The IF test must evaluate to a boolean.");
                }

                if (test->type() == cJSON_True) {
                    return eval_stream(t.if_().true_branch(), env);
                } else {
                    return eval_stream(t.if_().false_branch(), env);
                }
            }
            break;
        case Term::TABLE:
            {
                return eval_view(t.table(), env).stream;
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

boost::shared_ptr<scoped_cJSON_t> eval(const Term::Call &c, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    switch (c.builtin().type()) {
        //JSON -> JSON
        case Builtin::NOT:
            {
                boost::shared_ptr<scoped_cJSON_t> data = eval(c.args(0), env);

                if (data->get()->type == cJSON_False) {
                    data->get()->type = cJSON_True;
                } else if (data->get()->type == cJSON_True) {
                    data->get()->type = cJSON_False;
                } else {
                    throw runtime_exc_t("Not can only be called on a boolean");
                }
                return data;
            }
            break;
        case Builtin::GETATTR:
            {
                boost::shared_ptr<scoped_cJSON_t> data = eval(c.args(0), env);

                if (!data->type() == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
                }

                cJSON *value = data->GetObjectItem(c.builtin().attr().c_str());

                if (!value) {
                    throw runtime_exc_t("Object is missing attribute \"" + c.builtin().attr() + "\"");
                }

                return shared_scoped_json(cJSON_DeepCopy(value));
            }
            break;
        case Builtin::HASATTR:
            {
                boost::shared_ptr<scoped_cJSON_t> data = eval(c.args(0), env);

                if (!data->type() == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
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
            {
                boost::shared_ptr<scoped_cJSON_t> data = eval(c.args(0), env);

                if (!data->type() == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
                }

                boost::shared_ptr<scoped_cJSON_t> res = shared_scoped_json(cJSON_CreateObject());

                for (int i = 0; i < c.builtin().attrs_size(); ++i) {
                    cJSON *item = cJSON_DeepCopy(data->GetObjectItem(c.builtin().attrs(i).c_str()));
                    if (!item) {
                        throw runtime_exc_t("Attempting to pick missing attribute.");
                    } else {
                        res->AddItemToObject(item->string, item);
                    }
                }
                return res;
            }
            break;
        case Builtin::MAPMERGE:
            {
                boost::shared_ptr<scoped_cJSON_t> left  = eval(c.args(0), env),
                                                  right = eval(c.args(1), env);
                if (left->type() != cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
                }

                if (right->type() != cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
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
                boost::shared_ptr<scoped_cJSON_t> array  = eval(c.args(0), env);
                if (array->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(array->DeepCopy()));
                res->AddItemToArray(eval(c.args(1), env)->release());
                return res;
            }
            break;
        case Builtin::ARRAYCONCAT:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array1  = eval(c.args(0), env);
                if (array1->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }
                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> array2  = eval(c.args(1), env);
                if (array2->type() != cJSON_Array) {
                    throw runtime_exc_t("The second argument must be an array.");
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
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array = eval(c.args(0), env);
                if (array->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> start_json  = eval(c.args(1), env);
                if (start_json->type() != cJSON_Number) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }

                float float_start = start_json->get()->valuedouble;
                int start = (int)float_start;
                if (float_start != start) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }

                // Check third arg type
                boost::shared_ptr<scoped_cJSON_t> end_json  = eval(c.args(2), env);
                if (end_json->type() != cJSON_Number) {
                    throw runtime_exc_t("The third argument must be an integer.");
                }
                float float_end = end_json->get()->valuedouble;
                int stop = (int)float_end;
                if (float_end != stop) {
                    throw runtime_exc_t("The third argument must be an integer.");
                }

                int length = array->GetArraySize();

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
                    throw runtime_exc_t("The second argument cannot be greater than the third argument.");
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
                boost::shared_ptr<scoped_cJSON_t> array  = eval(c.args(0), env);
                if (array->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> index_json  = eval(c.args(1), env);
                if (index_json->type() != cJSON_Number) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }
                float float_index = index_json->get()->valuedouble;
                int index = (int)float_index;
                if (float_index != index) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }

                int length = array->GetArraySize();

                if (index < 0) {
                    index += length;
                }
                if (index < 0 || index >= length) {
                    throw runtime_exc_t("Array index out of bounds.");
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_DeepCopy(array->GetArrayItem(index))));
            }
            break;
        case Builtin::ARRAYLENGTH:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array  = eval(c.args(0), env);
                if (array->type() != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(array->GetArraySize())));
            }
            break;
        case Builtin::ADD:
            {
                double result = 0.0;

                for (int i = 0; i < c.args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                    if (arg->type() != cJSON_Number) {
                        throw runtime_exc_t("All operands to ADD must be numbers.");
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
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(0), env);
                    if (arg->type() != cJSON_Number) {
                        throw runtime_exc_t("All operands to SUBTRACT must be numbers.");
                    }
                    if (c.args_size() == 1) {
                        result = -arg->get()->valuedouble;  // (- x) is negate
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c.args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                        if (arg->type() != cJSON_Number) {
                            throw runtime_exc_t("All operands to SUBTRACT must be numbers.");
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
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                    if (arg->type() != cJSON_Number) {
                        throw runtime_exc_t("All operands of MULTIPLY must be numbers.");
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
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(0), env);
                    if (arg->type() != cJSON_Number) {
                        throw runtime_exc_t("All operands to SUBTRACT must be numbers.");
                    }
                    if (c.args_size() == 1) {
                        result = 1.0 / arg->get()->valuedouble;  // (/ x) is reciprocal
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c.args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                        if (arg->type() != cJSON_Number) {
                            throw runtime_exc_t("All operands to DIVIDE must be numbers.");
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
                boost::shared_ptr<scoped_cJSON_t> lhs = eval(c.args(0), env),
                    rhs = eval(c.args(1), env);
                if (lhs->type() != cJSON_Number || rhs->type() != cJSON_Number) {
                    throw runtime_exc_t("Both operands to MOD must be numbers.");
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateNumber(fmod(lhs->get()->valuedouble, rhs->get()->valuedouble))));
                return res;
            }
            break;
        case Builtin::COMPARE:
            {
                bool result = true;

                boost::shared_ptr<scoped_cJSON_t> lhs = eval(c.args(0), env);

                int type = lhs->type();

                if (type != cJSON_Number &&
                    type != cJSON_String &&
                    type != cJSON_True &&
                    type != cJSON_False)
                {
                    throw runtime_exc_t("Comparison is undefined for this type.");
                }

                for (int i = 1; i < c.args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> rhs = eval(c.args(i), env);

                    if (type == cJSON_Number) {
                        if (rhs->type() != type) {
                            throw runtime_exc_t("Cannot compare these types.");
                        }

                        double left = lhs->get()->valuedouble;
                        double right = rhs->get()->valuedouble;

                        switch (c.builtin().comparison()) {
                        case Builtin_Comparison_EQ:
                            result = (left == right);
                            break;
                        case Builtin_Comparison_NE:
                            result = (left != right);
                            break;
                        case Builtin_Comparison_LT:
                            result = (left < right);
                            break;
                        case Builtin_Comparison_LE:
                            result = (left <= right);
                            break;
                        case Builtin_Comparison_GT:
                            result = (left > right);
                            break;
                        case Builtin_Comparison_GE:
                            result = (left >= right);
                            break;
                        default:
                            crash("Unknown comparison operator.");
                            break;
                        }
                    } else if (type == cJSON_String) {
                        if (rhs->type() != type) {
                            throw runtime_exc_t("Cannot compare these types.");
                        }

                        char *left = lhs->get()->valuestring;
                        char *right = rhs->get()->valuestring;

                        int res = strcmp(left, right);
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
                        }
                    } else { // cJSON_True / cJSON_False
                        if (rhs->type() != cJSON_True && rhs->type() != cJSON_False) {
                            throw runtime_exc_t("Cannot compare these types.");
                        }

                        int lefttype = lhs->type();
                        int righttype = rhs->type();

                        bool eq = (lefttype == righttype);
                        bool lt = (lefttype == cJSON_False && righttype == cJSON_True);
                        bool gt = (lefttype == cJSON_True && righttype == cJSON_False);

                        switch (c.builtin().comparison()) {
                        case Builtin_Comparison_EQ:
                            result = eq;
                            break;
                        case Builtin_Comparison_NE:
                            result = !eq;
                            break;
                        case Builtin_Comparison_LT:
                            result = lt;
                            break;
                        case Builtin_Comparison_LE:
                            // False is always <= any bool
                            result = eq && lt;
                            break;
                        case Builtin_Comparison_GT:
                            result = gt;
                            break;
                        case Builtin_Comparison_GE:
                            result = eq && gt;
                            break;
                        default:
                            crash("Unknown comparison operator.");
                            break;
                        }
                    }

                    if (!result) {
                        break;
                    }

                    lhs = rhs;
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateBool(result)));
                return res;
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
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);
                int length = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    ++length;
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(length)));
            }
            break;
        case Builtin::NTH:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> index_json  = eval(c.args(1), env);
                if (index_json->type() != cJSON_Number) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }
                float index_float = index_json->get()->valuedouble;
                int index = (int)index_float;
                if (index_float != index || index < 0) {
                    throw runtime_exc_t("The second argument must be a nonnegative integer.");
                }

                boost::shared_ptr<scoped_cJSON_t> json;
                for (int i = 0; i <= index; ++i) {
                    json = stream->next();
                    if (!json) {
                        throw runtime_exc_t("Index out of bounds.");
                    }
                }

                if (!json) {
                    throw runtime_exc_t("Index out of bounds");
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(json->DeepCopy()));
            }
            break;
        case Builtin::STREAMTOARRAY:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    res->AddItemToArray(json->DeepCopy());
                }

                return res;
            }
            break;
        case Builtin::REDUCE:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                throw runtime_exc_t("Not implemented reduce");
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
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                    if (arg->type() != cJSON_False && arg->type() != cJSON_True) {
                        throw runtime_exc_t("All operands to ALL must be booleans.");
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
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                    if (arg->type() != cJSON_False && arg->type() != cJSON_True) {
                        throw runtime_exc_t("All operands to ANY must be booleans.");
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
    predicate_t(const Predicate &_pred, runtime_environment_t *_env)
        : pred(_pred), env(_env)
    { }
    bool operator()(boost::shared_ptr<scoped_cJSON_t> json) {
        variable_val_scope_t::new_scope_t scope_maker(&env->scope);
        env->scope.put_in_scope(pred.arg(), json);
        boost::shared_ptr<scoped_cJSON_t> a_bool = eval(pred.body(), env);

        if (a_bool->type() == cJSON_True) {
            return true;
        } else if (a_bool->type() == cJSON_False) {
            return false;
        } else {
            throw runtime_exc_t("Predicate failed to evaluate to a bool");
        }
    }
private:
    Predicate pred;
    runtime_environment_t *env;
};

class not_t {
public:
    explicit not_t(const predicate_t &_pred)
        : pred(_pred)
    { }
    bool operator()(boost::shared_ptr<scoped_cJSON_t> json) {
        bool res = pred(json);
        return !res;
    }
private:
    predicate_t pred;
};

class ordering_t {
public:
    ordering_t(const google::protobuf::RepeatedPtrField<Builtin::OrderBy> &_order)
        : order(_order)
    { }

    //returns true if x < y according to the ordering
    bool operator()(const boost::shared_ptr<scoped_cJSON_t> &x, const boost::shared_ptr<scoped_cJSON_t> &y) {
        for (int i = 0; i < order.size(); ++i) {
            const Builtin::OrderBy& cur = order.Get(i);

            cJSON *a = cJSON_GetObjectItem(x->get(), cur.attr().c_str());
            cJSON *b = cJSON_GetObjectItem(y->get(), cur.attr().c_str());

            if (a == NULL || b == NULL) {
                throw runtime_exc_t("OrderBy encountered a row missing attr " + cur.attr());
            }

            int cmp = cJSON_cmp(a, b);
            if (cmp) {
                return (cmp > 0) ^ cur.ascending();
            }
        }

        return false;
    }

private:
    const google::protobuf::RepeatedPtrField<Builtin::OrderBy> &order;
};

boost::shared_ptr<scoped_cJSON_t> map(std::string arg, const Term &term, runtime_environment_t *env, boost::shared_ptr<scoped_cJSON_t> val) {
    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(arg, val);
    return eval(term, env);
}

boost::shared_ptr<json_stream_t> concatmap(std::string arg, const Term &term, runtime_environment_t *env, boost::shared_ptr<scoped_cJSON_t> val) {
    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
    env->scope.put_in_scope(arg, val);
    return eval_stream(term, env);
}

boost::shared_ptr<json_stream_t> eval_stream(const Term::Call &c, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    switch (c.builtin().type()) {
        //JSON -> JSON
        case Builtin::NOT:
        case Builtin::GETATTR:
        case Builtin::HASATTR:
        case Builtin::PICKATTRS:
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
                std::map<boost::shared_ptr<scoped_cJSON_t>, boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less> groups;
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    boost::shared_ptr<scoped_cJSON_t> group_mapped_row, value_mapped_row, reduced_row;

                    // Figure out which group we belong to
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                        env->scope.put_in_scope(c.builtin().grouped_map_reduce().group_mapping().arg(), json);
                        group_mapped_row = eval(c.builtin().grouped_map_reduce().group_mapping().body(), env);
                    }

                    // Map the value for comfy reduction goodness
                    {
                        variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                        env->scope.put_in_scope(c.builtin().grouped_map_reduce().value_mapping().arg(), json);
                        value_mapped_row = eval(c.builtin().grouped_map_reduce().value_mapping().body(), env);
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
                                                    eval(c.builtin().grouped_map_reduce().reduction().base(), env));
                        }
                        env->scope.put_in_scope(c.builtin().grouped_map_reduce().reduction().var2(), value_mapped_row);

                        reduced_row = eval(c.builtin().grouped_map_reduce().reduction().body(), env);
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
                predicate_t p(c.builtin().filter().predicate(), env);
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);
                return boost::shared_ptr<json_stream_t>(new filter_stream_t<predicate_t>(stream, p));
            }
            break;
        case Builtin::MAP:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                return boost::shared_ptr<json_stream_t>(new mapping_stream_t<boost::function<boost::shared_ptr<scoped_cJSON_t>(boost::shared_ptr<scoped_cJSON_t>)> >(
                                                                stream, boost::bind(&map, c.builtin().map().mapping().arg(), c.builtin().map().mapping().body(), env, _1)));
            }
            break;
        case Builtin::CONCATMAP:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                return boost::shared_ptr<json_stream_t>(new concat_mapping_stream_t<boost::function<boost::shared_ptr<json_stream_t>(boost::shared_ptr<scoped_cJSON_t>)> >(
                                                                stream, boost::bind(&concatmap, c.builtin().map().mapping().arg(), c.builtin().map().mapping().body(), env, _1)));
            }
            break;
        case Builtin::ORDERBY:
            {
                ordering_t o(c.builtin().order_by());
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                boost::shared_ptr<in_memory_stream_t> sorted_stream(new in_memory_stream_t(stream));
                sorted_stream->sort(o);
                return sorted_stream;
            }
            break;
        case Builtin::DISTINCT:
            {
                std::set<boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less> seen;

                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    seen.insert(json);
                }

                return boost::shared_ptr<in_memory_stream_t>(new in_memory_stream_t(seen.begin(), seen.end()));
            }
            break;
        case Builtin::LIMIT:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> limit_json  = eval(c.args(1), env);
                if (limit_json->type() != cJSON_Number) {
                    throw runtime_exc_t("The limit must be a nonnegative integer.");
                }
                float limit_float = limit_json->get()->valuedouble;
                int limit = (int)limit_float;
                if (limit_float != limit || limit < 0) {
                    throw runtime_exc_t("The limit must be a nonnegative integer.");
                }

                return boost::shared_ptr<json_stream_t>(new limit_stream_t(stream, limit));
            }
            break;
        case Builtin::UNION:
            {
                union_stream_t::stream_list_t streams;

                streams.push_back(eval_stream(c.args(0), env));
                streams.push_back(eval_stream(c.args(1), env));

                return boost::shared_ptr<json_stream_t>(new union_stream_t(streams));
            }
            break;
        case Builtin::ARRAYTOSTREAM:
            {

                boost::shared_ptr<scoped_cJSON_t> array = eval(c.args(0), env);
                json_array_iterator_t it(array->get());

                return boost::shared_ptr<json_stream_t>(new in_memory_stream_t(it));
            }
            break;
        case Builtin::RANGE:
            throw runtime_exc_t("Unimplemented: Builtin::RANGE");
            break;
        default:
            crash("unreachable");
            break;
    }
    crash("unreachable");

}

namespace_repo_t<rdb_protocol_t>::access_t eval(const TableRef &t, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    boost::optional<std::pair<namespace_id_t, deletable_t<namespace_semilattice_metadata_t<rdb_protocol_t> > > > namespace_info =
        env->semilattice_metadata->get().rdb_namespaces.get_namespace_by_name(t.table_name());

    if (namespace_info) {
        return namespace_repo_t<rdb_protocol_t>::access_t(env->ns_repo, namespace_info->first, &env->interruptor);
    } else {
        throw runtime_exc_t(strprintf("Namespace %s either not found, ambigious or namespace metadata in conflict.", t.table_name().c_str()));
    }
}

view_t eval_view(const Term::Table &t, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(t.table_ref(), env);
    key_range_t range = rdb_protocol_t::region_t::universe();
    rdb_protocol_t::rget_read_t rget_read(range);
    rdb_protocol_t::read_t read(rget_read);
    rdb_protocol_t::read_response_t res = ns_access.get_namespace_if()->read(read, order_token_t::ignore, &env->interruptor);
    rdb_protocol_t::rget_read_response_t *p_res = boost::get<rdb_protocol_t::rget_read_response_t>(&res.response);
    rassert(p_res);
    boost::shared_ptr<json_stream_t> stream(new in_memory_stream_t(p_res->data.begin(), p_res->data.end()));
    return view_t(ns_access, stream);
}

} //namespace query_language
