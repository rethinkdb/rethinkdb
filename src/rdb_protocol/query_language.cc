#include "rdb_protocol/query_language.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>
#include <math.h>

#include "http/json.hpp"

#define CHECK(x) if (!(x)) { return false; }
#define CHECK_WELL_DEFINED(x) CHECK(is_well_defined(x))

bool is_well_defined(const VarTermTuple &v) {
    return is_well_defined(v.term());
}

bool is_well_defined(const Term &t) {
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    t.GetReflection()->ListFields(t, &fields);
    int field_count = fields.size();

    CHECK(field_count <= 2);

    switch (t.type()) {
    case Term::VAR:
        CHECK(t.has_var());
        break;
    case Term::LET:
        CHECK(t.has_let());
        for (int i = 0; i < t.let().binds_size(); ++i) {
            CHECK_WELL_DEFINED(t.let().binds(i));
        }
        CHECK_WELL_DEFINED(t.let().expr());
        break;
    case Term::CALL:
        CHECK(t.has_call());
        CHECK_WELL_DEFINED(t.call().builtin());
        for (int i = 0; i < t.call().args_size(); ++i) {
            CHECK_WELL_DEFINED(t.call().args(i));
        }
        break;
    case Term::IF:
        CHECK(t.has_if_());
        CHECK_WELL_DEFINED(t.if_().test());
        CHECK_WELL_DEFINED(t.if_().true_branch());
        CHECK_WELL_DEFINED(t.if_().false_branch());
        break;
    case Term::ERROR:
        CHECK(t.has_error());
        break;
    case Term::NUMBER:
        CHECK(t.has_number());
        break;
    case Term::STRING:
        CHECK(t.has_valuestring());
        break;
    case Term::JSON:
        CHECK(t.has_jsonstring());
        break;
    case Term::BOOL:
        CHECK(t.has_valuebool());
        break;
    case Term::JSON_NULL:
        CHECK(field_count == 1); // null term has only a type field
        break;
    case Term::ARRAY:
        if (t.array_size() == 0) { // empty arrays are valid
            CHECK(field_count == 1);
        }
        for (int i = 0; i < t.array_size(); ++i) {
            CHECK_WELL_DEFINED(t.array(i));
        }
        break;
    case Term::OBJECT:
        if (t.object_size() == 0) { // empty objects are valid
            CHECK(field_count == 1);
        }
        for (int i = 0; i < t.object_size(); ++i) {
            CHECK_WELL_DEFINED(t.object(i).term());
        }
        break;
    case Term::GETBYKEY:
        CHECK(t.has_get_by_key());
        CHECK_WELL_DEFINED(t.get_by_key().key());
        break;
    case Term::TABLE:
        CHECK(t.has_table());
        break;
    default:
        return false;
    }

    return true;
}

bool is_well_defined(const Builtin &b) {
    std::vector<const google::protobuf::FieldDescriptor *> fields;

    b.GetReflection()->ListFields(b, &fields);

    int field_count = fields.size();

    if (field_count > 2) {
        return false;
    }

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
    case Builtin::JAVASCRIPT:
    case Builtin::JAVASCRIPTRETURNINGSTREAM:
    case Builtin::ANY:
    case Builtin::ALL:
        // these builtins only have
        // Builtin.type set
        CHECK(field_count == 1);
        break;
    case Builtin::COMPARE:
        CHECK(b.has_comparison());
        break;
    case Builtin::GETATTR:
    case Builtin::HASATTR:
        CHECK(b.has_attr());
        break;
    case Builtin::PICKATTRS:
        CHECK(b.attrs_size());
        break;
    case Builtin::FILTER:
        CHECK(b.has_filter());
        CHECK_WELL_DEFINED(b.filter().predicate());
        break;
    case Builtin::MAP:
        CHECK(b.has_map());
        CHECK_WELL_DEFINED(b.map().mapping());
        break;
    case Builtin::CONCATMAP:
        CHECK(b.has_concat_map());
        CHECK_WELL_DEFINED(b.concat_map().mapping());
        break;
    case Builtin::ORDERBY:
        CHECK(b.has_order_by());
        CHECK_WELL_DEFINED(b.order_by().mapping());
        break;
    case Builtin::REDUCE:
        CHECK(b.has_reduce());
        CHECK_WELL_DEFINED(b.reduce());
        break;
    case Builtin::GROUPEDMAPREDUCE:
        CHECK(b.has_grouped_map_reduce());
        CHECK_WELL_DEFINED(b.grouped_map_reduce().group_mapping());
        CHECK_WELL_DEFINED(b.grouped_map_reduce().map_reduce().change_mapping());
        CHECK_WELL_DEFINED(b.grouped_map_reduce().map_reduce().reduction());
        break;
    case Builtin::MAPREDUCE:
        CHECK(b.has_map_reduce());
        CHECK_WELL_DEFINED(b.map_reduce().change_mapping());
        CHECK_WELL_DEFINED(b.map_reduce().reduction());
        break;
    case Builtin::RANGE:
        CHECK(b.has_range());
        if (b.range().has_lowerbound()) {
            CHECK_WELL_DEFINED(b.range().lowerbound());
        }
        if (b.range().has_upperbound()) {
            CHECK_WELL_DEFINED(b.range().lowerbound());
        }
        break;
    default:
        return false;
    }

    return true;
}

bool is_well_defined(const Reduction &r) {
    CHECK_WELL_DEFINED(r.base())
    CHECK_WELL_DEFINED(r.body())
    return true;
}

bool is_well_defined(const Mapping &m) {
    CHECK_WELL_DEFINED(m.body());
    return true;
}

bool is_well_defined(const Predicate &p) {
    CHECK_WELL_DEFINED(p.body());
    return true;
}

bool is_well_defined(const ReadQuery &r) {
    return is_well_defined(r.term());
}

bool is_well_defined(const WriteQuery &w) {
    std::vector<const google::protobuf::FieldDescriptor *> fields;
    w.GetReflection()->ListFields(w, &fields);
    CHECK(fields.size() == 2);

    switch (w.type()) {
    case WriteQuery::UPDATE:
        CHECK(w.has_update());
        CHECK_WELL_DEFINED(w.update().view());
        CHECK_WELL_DEFINED(w.update().mapping());
        break;
    case WriteQuery::DELETE:
        CHECK(w.has_delete_());
        CHECK_WELL_DEFINED(w.delete_().view());
        break;
    case WriteQuery::MUTATE:
        CHECK(w.has_mutate());
        CHECK_WELL_DEFINED(w.mutate().view());
        CHECK_WELL_DEFINED(w.mutate().mapping());
        break;
    case WriteQuery::INSERT:
        CHECK(w.has_insert());
        for (int i = 0; i < w.insert().terms_size(); ++i) {
            CHECK_WELL_DEFINED(w.insert().terms(i));
        }
        break;
    case WriteQuery::INSERTSTREAM:
        CHECK(w.has_insert_stream());
        CHECK_WELL_DEFINED(w.insert_stream().stream());
        break;
    case WriteQuery::FOREACH:
        CHECK(w.has_for_each());
        CHECK_WELL_DEFINED(w.for_each().stream());
        for (int i = 0; i < w.for_each().queries_size(); ++i) {
            CHECK_WELL_DEFINED(w.for_each().queries(i));
        }
        break;
    case WriteQuery::POINTUPDATE:
        CHECK(w.has_point_update());
        CHECK_WELL_DEFINED(w.point_update().key());
        CHECK_WELL_DEFINED(w.point_update().mapping());
        break;
    case WriteQuery::POINTDELETE:
        CHECK(w.has_point_delete());
        CHECK_WELL_DEFINED(w.point_delete().key());
        break;
    case WriteQuery::POINTMUTATE:
        CHECK(w.has_point_mutate());
        CHECK_WELL_DEFINED(w.point_mutate().key());
        CHECK_WELL_DEFINED(w.point_mutate().mapping());
        break;
    default:
        return false;
    }

    return true;
}

bool is_well_defined(const Query &q) {
    switch (q.type()) {
    case Query::READ:
        CHECK(q.has_read_query());
        CHECK(!q.has_write_query());
        CHECK_WELL_DEFINED(q.read_query());
        break;
    case Query::WRITE:
        CHECK(q.has_write_query());
        CHECK(!q.has_read_query());
        CHECK_WELL_DEFINED(q.write_query());
        break;
    default:
        return false;
    }

    return true;
}

namespace query_language {
const type_t get_type(const Term &t, variable_type_scope_t *scope) {
    switch (t.type()) {
        case Term::VAR:
            if (scope->is_in_scope(t.var())) {
                return scope->get(t.var());
            } else {
                return error_t(strprintf("Symbol %s is not in scope\n", t.var().c_str()));
            }
            break;
        case Term::LET:
            {
                scope->push(); //create a new scope
                for (int i = 0; i < t.let().binds_size(); ++i) {
                    scope->put_in_scope(t.let().binds(i).var(), get_type(t.let().binds(i).term(), scope));
                }
                type_t res = get_type(t.let().expr(), scope);
                scope->pop();
                return res;
                break;
            }
        case Term::CALL:
            {
                function_t signature = get_type(t.call().builtin(), scope);
                if (!signature.is_variadic()) {
                    int n_args = signature.get_n_args();
                    if (t.call().args_size() > n_args) {
                        return error_t(strprintf("Too many arguments passed to function. Expected %d but got %d", n_args, t.call().args_size())); //TODO would be nice to have function names attached to errors
                    } else if (t.call().args_size() < n_args) {
                        return error_t(strprintf("Too few  arguments passed to function. Expected %d but got %d", n_args, t.call().args_size())); //TODO would be nice to have function names attached to errors
                    }
                }
                for (int i = 0; i < t.call().args_size(); ++i) {
                    if (!(get_type(t.call().args(i), scope) == signature.get_arg_type(i))) {
                        return error_t("Type mismatch in function call\n"); //Need descriptions of types to give a more informative message
                    }
                }
                return signature.get_return_type();
                break;
            }
        case Term::IF:
            {
                if (!(get_type(t.if_().test(), scope) == Type::JSON)) {
                    return error_t("Test in an if must be JSON\n");
                }
                type_t true_branch  = get_type(t.if_().true_branch(), scope),
                       false_branch = get_type(t.if_().false_branch(), scope);
                if (!(true_branch == false_branch)) {
                    return error_t("Mismatch between true and false branch types.");
                } else {
                    return true_branch;
                }
                break;
            }
        case Term::ERROR:
            return Type::ERROR;
        case Term::NUMBER:
        case Term::STRING:
        case Term::JSON:
        case Term::BOOL:
        case Term::JSON_NULL:
            return Type::JSON;
        case Term::ARRAY:
            {
                for (int i = 0; i < t.array_size(); ++i) {
                    if (!(get_type(t.array(i), scope) == Type::JSON)) {
                        return error_t("Type mismatch in array\n"); //Need descriptions of types to give a more informative message
                    }
                }
                return Type::JSON;
            }
            break;
        case Term::OBJECT:
            return Type::JSON;
        case Term::GETBYKEY:
            if (get_type(t.get_by_key().key(), scope) == Type::JSON) {
                return Type::JSON;
            } else {
                return error_t("Key must be a json value.");
            }
            break;
        case Term::TABLE:
            //no way for this to be incorrect
            return Type::VIEW;
            break;
        default:
            crash("unreachable");
            break;
    }
    crash("unreachable");
}

function_t::function_t(const type_t& _arg_type, int _n_args, const type_t& _return_type)
    : n_args(_n_args), return_type(_return_type) {
    arg_type[0] = _arg_type;
    for (int i = 1; i < _n_args; ++i) {
        arg_type[i] = _arg_type;
    }
}

function_t::function_t(const type_t& _arg1_type, const type_t& _arg2_type, const type_t& _return_type)
    : n_args(2), return_type(_return_type) {
    arg_type[0] = _arg1_type;
    arg_type[1] = _arg2_type;
}

const type_t& function_t::get_arg_type(int n) const {
    if (n >= 0 && n < n_args) {
        return arg_type[n];
    } else {
        return arg_type[0];
    }
}

const type_t& function_t::get_return_type() const {
    return return_type;
}

bool function_t::is_variadic() const {
    return n_args == -1;
}

int function_t::get_n_args() const {
    return n_args;
}

const function_t get_type(const Builtin &b, variable_type_scope_t *) {
    switch (b.type()) {
        //JSON -> JSON
        case Builtin::NOT:
        case Builtin::GETATTR:
        case Builtin::HASATTR:
        case Builtin::PICKATTRS:
        case Builtin::ARRAYLENGTH:
        case Builtin::JAVASCRIPT:
            return function_t(Type::JSON, 1, Type::JSON);
            break;
        case Builtin::MAPMERGE:
        case Builtin::ARRAYAPPEND:
        case Builtin::ARRAYCONCAT:
        case Builtin::ARRAYNTH:
        case Builtin::MODULO:
            return function_t(Type::JSON, 2, Type::JSON);
            break;
        case Builtin::ADD:
        case Builtin::SUBTRACT:
        case Builtin::MULTIPLY:
        case Builtin::DIVIDE:
        case Builtin::COMPARE:
        case Builtin::ANY:
        case Builtin::ALL:
            return function_t(Type::JSON, -1, Type::JSON);  // variadic JSON type
            break;
        case Builtin::ARRAYSLICE:
            return function_t(Type::JSON, 3, Type::JSON);
            break;
        case Builtin::FILTER:
        case Builtin::MAP:
        case Builtin::CONCATMAP:
        case Builtin::ORDERBY:
        case Builtin::DISTINCT:
            return function_t(Type::STREAM, 1, Type::STREAM);
            break;
        case Builtin::LIMIT:
            return function_t(Type::STREAM, Type::JSON, Type::STREAM);
            break;
        case Builtin::NTH:
            return function_t(Type::STREAM, Type::JSON, Type::JSON);
            break;
        case Builtin::LENGTH:
        case Builtin::STREAMTOARRAY:
        case Builtin::REDUCE:
        case Builtin::GROUPEDMAPREDUCE:
        case Builtin::MAPREDUCE:
            return function_t(Type::STREAM, 1, Type::JSON);
            break;
        case Builtin::UNION:
            return function_t(Type::STREAM, 2, Type::STREAM);
            break;
        case Builtin::ARRAYTOSTREAM:
        case Builtin::JAVASCRIPTRETURNINGSTREAM:
            return function_t(Type::JSON, 1, Type::STREAM);
            break;
        case Builtin::RANGE:
            return function_t(Type::VIEW, 1, Type::VIEW);
            break;
        default:
            crash("unreachable");
            break;
    }
}

const type_t get_type(const Reduction &r, variable_type_scope_t *scope) {
    if (!(get_type(r.base(), scope) == Type::JSON)) {
        return type_t(error_t());
    }

    new_scope_t scope_maker(scope);
    scope->put_in_scope(r.var1(), Type::JSON);
    scope->put_in_scope(r.var2(), Type::JSON);

    if (!(get_type(r.body(), scope) == Type::JSON)) {
        return type_t(error_t());
    } else {
        return Type::JSON;
    }
}

const type_t get_type(const Mapping &m, variable_type_scope_t *scope) {
    new_scope_t scope_maker(scope);
    scope->put_in_scope(m.arg(), Type::JSON);

    if (!(get_type(m.body(), scope) == Type::JSON)) {
        return type_t(error_t());
    } else {
        return Type::JSON;
    }
}

const type_t get_type(const Predicate &p, variable_type_scope_t *scope) {
    new_scope_t scope_maker(scope);
    scope->put_in_scope(p.arg(), Type::JSON);

    if (!(get_type(p.body(), scope) == Type::JSON)) {
        return type_t(error_t());
    } else {
        return Type::JSON;
    }
}

const type_t get_type(const ReadQuery &r, variable_type_scope_t *scope) {
    type_t res = get_type(r.term(), scope);
    if (res == Type::JSON ||
        res == Type::STREAM) {
        return Type::READ;
    } else {
        return error_t("ReadQueries must produce either JSON or a STREAM.");
    }
}

const type_t get_type(const WriteQuery &w, variable_type_scope_t *scope) {
    switch (w.type()) {
        case WriteQuery::UPDATE:
            if (get_type(w.update().view(), scope) == Type::VIEW &&
                get_type(w.update().mapping(), scope) == Type::JSON) {
                return Type::WRITE;
            }
            break;
        case WriteQuery::DELETE:
            if (get_type(w.update().view(), scope) == Type::VIEW) {
                return Type::WRITE;
            }
            break;
        case WriteQuery::MUTATE:
            if (get_type(w.mutate().view(), scope) == Type::VIEW &&
                get_type(w.mutate().mapping(), scope) == Type::JSON) {
                return Type::WRITE;
            }
            break;
        case WriteQuery::INSERT:
            for (int i = 0; i < w.insert().terms_size(); ++i) {
                if (!(get_type(w.insert().terms(i), scope) == Type::JSON)) {
                    return type_t(error_t("Trying to insert a non JSON term"));
                }
            }
            return Type::WRITE;
            break;
        case WriteQuery::INSERTSTREAM:
            if (!(get_type(w.insert_stream().stream(), scope) == Type::STREAM)) {
                return Type::WRITE;
            }
            break;
        case WriteQuery::FOREACH:
            {
                if (!(get_type(w.for_each().stream(), scope) == Type::STREAM)) {
                    return type_t(error_t("Must pass a stream in to a FOREACH query."));
                }

                new_scope_t scope_maker(scope);
                scope->put_in_scope(w.for_each().var(), Type::JSON);
                for (int i = 0; i < w.for_each().queries_size(); ++i) {
                    if (!(get_type(w.for_each().queries(i), scope) == Type::WRITE)) {
                        return type_t(error_t("Queries passed to a foreach must all be write queries\n"));
                    }
                }
                return Type::WRITE;
            }
            break;
        case WriteQuery::POINTUPDATE:
            if (!(get_type(w.point_update().key(), scope) == Type::JSON) ||
                !(get_type(w.point_update().mapping(), scope) == Type::JSON)) {
                return type_t(error_t("Key and mapping must both be of type JSON\n"));
            }
            return Type::WRITE;
            break;
        case WriteQuery::POINTDELETE:
            if (!(get_type(w.point_delete().key(), scope) == Type::JSON)) {
                return type_t(error_t("Key must be of type JSON\n"));
            }
            return Type::WRITE;
            break;
        case WriteQuery::POINTMUTATE:
            if (!(get_type(w.point_mutate().key(), scope) == Type::JSON) ||
                !(get_type(w.point_mutate().mapping(), scope) == Type::JSON)) {
                return type_t(error_t("Key and mapping must both be of type JSON\n"));
            }
            return Type::WRITE;
            break;
        default:
            break;
    }
    crash("Unreachable");
}

const type_t get_type(const Query &q, variable_type_scope_t *scope) {
    switch (q.type()) {
        case Query::READ:
            if (!(get_type(q.read_query(), scope) == Type::READ)) {
                return type_t(error_t("Malformed read."));
            }
            return Type::QUERY;
            break;
        case Query::WRITE:
            if (!(get_type(q.write_query(), scope) == Type::WRITE)) {
                return type_t(error_t("Malformed write."));
            }
            return Type::QUERY;
            break;
        default:
            crash("unreachable");
    }
    crash("unreachable");
}

Response eval(const Query &q, runtime_environment_t *env) {
    switch (q.type()) {
        case Query::READ:
            return eval(q.read_query(), env);
            break;
        case Query::WRITE:
            return eval(q.write_query(), env);
            break;
        default:
            crash("unreachable");
    }
    crash("unreachable");
}

Response eval(const ReadQuery &r, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    Response res;

    type_t type = get_type(r.term(), &env->type_scope);

    if (type == Type::JSON) {
        boost::shared_ptr<scoped_cJSON_t> json = eval(r.term(), env);
        res.add_response(cJSON_print_std_string(json->get()));
    } else if (type == Type::STREAM) {
        boost::shared_ptr<json_stream_t> stream = eval_stream(r.term(), env);

        while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
            res.add_response(cJSON_print_std_string(json->get()));
        }
    } else {
        unreachable("The type checker should have caught use before we got" \
                "here. (A read query with a term that doesn't evaluate to JSON" \
                    "or STREAM isn't allowed.");
    }

    res.set_status_code(0);
    res.set_token(0);
    return res;
}

void insert(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> data, runtime_environment_t *env) {
    if (!cJSON_GetObjectItem(data->get(), "id")) {
        throw runtime_exc_t("Must have a field named id.");
    }

    rdb_protocol_t::write_t write(rdb_protocol_t::point_write_t(store_key_t(cJSON_print_std_string(cJSON_GetObjectItem(data->get(), "id"))), data));
    ns_access.get_namespace_if()->write(write, order_token_t::ignore, &env->interruptor);
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, cJSON *id, runtime_environment_t *env) {
    rdb_protocol_t::write_t write(rdb_protocol_t::point_delete_t(store_key_t(cJSON_print_std_string(id))));
    ns_access.get_namespace_if()->write(write, order_token_t::ignore, &env->interruptor);
}

void point_delete(namespace_repo_t<rdb_protocol_t>::access_t ns_access, boost::shared_ptr<scoped_cJSON_t> id, runtime_environment_t *env) {
    point_delete(ns_access, id->get(), env);
}

Response eval(const WriteQuery &w, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    switch (w.type()) {
        case WriteQuery::UPDATE:
            {
                view_t view = eval_view(w.update().view().table(), env);

                int modified = 0,
                    error = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope);

                    env->scope.put_in_scope(w.update().mapping().arg(), json);
                    boost::shared_ptr<scoped_cJSON_t> val = eval(w.update().mapping().body(), env);
                    if (!cJSON_Equal(cJSON_GetObjectItem(json->get(), "id"), 
                                     cJSON_GetObjectItem(val->get(), "id"))) {
                        error++;
                    } else {
                        insert(view.access, val, env);
                        modified++;
                    }
                }

                Response res;
                res.set_status_code(0);
                res.set_token(0);
                res.add_response(strprintf("Modified %d rows.", modified));
                res.add_response(strprintf("%d errors.", error));
                return res;
            }
            break;
        case WriteQuery::DELETE:
            {
                view_t view = eval_view(w.delete_().view().table(), env);

                int deleted = 0;
                while (boost::shared_ptr<scoped_cJSON_t> json = view.stream->next()) {
                    point_delete(view.access, cJSON_GetObjectItem(json->get(), "id"), env);
                    deleted++;
                }

                Response res;
                res.set_status_code(0);
                res.set_token(0);
                res.add_response(strprintf("Deleted %d rows.", deleted));
                return res;
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

                    if (val->get()->type == cJSON_NULL) {
                        point_delete(view.access, cJSON_GetObjectItem(json->get(), "id"), env);
                        ++deleted;
                    } else {
                        insert(view.access, eval(w.update().mapping().body(), env), env);
                        ++modified;
                    }
                }

                Response res;
                res.set_status_code(0);
                res.set_token(0);
                res.add_response(strprintf("Deleted %d rows.", deleted));
                res.add_response(strprintf("Modified %d rows.", modified));
                return res;
            }
            break;
        case WriteQuery::INSERT:
            {
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.insert().table_ref(), env);
                for (int i = 0; i < w.insert().terms_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> data = eval(w.insert().terms(i), env);

                    insert(ns_access, data, env);
                }
                Response res;
                res.set_status_code(0);
                res.set_token(0);
                res.add_response(strprintf("Inserted %d rows.", w.insert().terms_size()));
                return res;
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

                Response res;
                res.set_status_code(0);
                res.set_token(0);
                res.add_response(strprintf("Inserted %d rows.", inserted));
                return res;
            }
            break;
        case WriteQuery::FOREACH:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(w.for_each().stream(), env);

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                    env->scope.put_in_scope(w.for_each().var(), json);

                    for (int i = 0; i < w.for_each().queries_size(); ++i) {
                        eval(w.for_each().queries(i), env);
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

                rassert(is_well_defined(get));

                boost::shared_ptr<scoped_cJSON_t> original_val = eval(get, env);
                new_val_scope_t scope_maker(&env->scope);
                env->scope.put_in_scope(w.point_update().mapping().arg(), original_val);

                boost::shared_ptr<scoped_cJSON_t> new_val = eval(w.point_update().mapping().body(), env);

                /* Now we insert the new value. */
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.point_update().table_ref(), env);

                insert(ns_access, new_val, env);

                Response res;
                res.set_status_code(0);
                res.set_token(0);
                res.add_response("Updated 1 rows.");
                return res;
            }
            break;
        case WriteQuery::POINTDELETE:
            {
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(w.point_delete().table_ref(), env);
                boost::shared_ptr<scoped_cJSON_t> id = eval(w.point_delete().key(), env);
                point_delete(ns_access, id, env);

                Response res;
                res.set_status_code(0);
                res.set_token(0);
                res.add_response("Deleted 1 rows.");

                return res;
            }
            break;
        case WriteQuery::POINTMUTATE:
            break;
        default:
            unreachable();
            break;
    }

    Response res;
    res.set_status_code(-3);
    res.set_token(0);
    res.add_response("Unimplemented");
    return res;
}

//This doesn't create a scope for the evals
void eval_let_binds(const Term::Let &let, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    // Go through the bindings in a let and add them one by one
    for (int i = 0; i < let.binds_size(); ++i) {
        type_t type = get_type(let.binds(i).term(), &env->type_scope);

        if (type == Type::JSON) {
            env->scope.put_in_scope(let.binds(i).var(),
                    eval(let.binds(i).term(), env));
        } else if (type == Type::STREAM) {
            env->stream_scope.put_in_scope(let.binds(i).var(),
                    eval_stream(let.binds(i).term(), env));
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
                if (test->get()->type != cJSON_True && test->get()->type != cJSON_False) {
                    throw runtime_exc_t("The IF test must evaluate to a boolean.");
                }

                boost::shared_ptr<scoped_cJSON_t> res;
                if (test->get()->type == cJSON_True) {
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
                    cJSON_AddItemToArray(res->get(), eval(t.array(i), env)->release());
                }
                return res;
            }
            break;
        case Term::OBJECT:
            {
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateObject()));
                for (int i = 0; i < t.object_size(); ++i) {
                    std::string item_name(t.object(i).var());
                    cJSON_AddItemToObject(res->get(), item_name.c_str(), eval(t.object(i).term(), env)->release());
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
                rdb_protocol_t::read_t read(rdb_protocol_t::point_read_t(store_key_t(cJSON_print_std_string(key->get()))));
                rdb_protocol_t::read_response_t res = ns_access.get_namespace_if()->read(read, order_token_t::ignore, &env->interruptor);

                rdb_protocol_t::point_read_response_t *p_res = boost::get<rdb_protocol_t::point_read_response_t>(&res.response);
                return p_res->data;
                break;
            }
        case Term::TABLE:
            throw runtime_exc_t("Term::TABLE must be evaluated with eval_stream or eval_view");
            break;
        default:
            crash("unreachable");
            break;
    }
    crash("unreachable");
}

boost::shared_ptr<json_stream_t> eval_stream(const Term &t, runtime_environment_t *env) THROWS_ONLY(runtime_exc_t) {
    switch (t.type()) {
        case Term::VAR:
            return env->stream_scope.get(t.var());
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
                if (test->get()->type != cJSON_True && test->get()->type != cJSON_False) {
                    throw runtime_exc_t("The IF test must evaluate to a boolean.");
                }

                if (test->get()->type == cJSON_True) {
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
            unreachable("eval_stream called on a function that does not return a stream (use eval instead).\n");
            break;
        default:
            crash("unreachable");
            break;
    }
    crash("unreachable");

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

                if (!data->get()->type == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
                }

                cJSON *value = cJSON_GetObjectItem(data->get(), c.builtin().attr().c_str());

                if (!value) {
                    throw runtime_exc_t("Object is missing attribute \"" + c.builtin().attr() + "\"");
                }

                return shared_scoped_json(cJSON_DeepCopy(value));
            }
            break;
        case Builtin::HASATTR:
            {
                boost::shared_ptr<scoped_cJSON_t> data = eval(c.args(0), env);

                if (!data->get()->type == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
                }

                cJSON *attr = cJSON_GetObjectItem(data->get(), c.builtin().attr().c_str());

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

                if (!data->get()->type == cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
                }

                boost::shared_ptr<scoped_cJSON_t> res = shared_scoped_json(cJSON_CreateObject());

                for (int i = 0; i < c.builtin().attrs_size(); ++i) {
                    cJSON *item = cJSON_DeepCopy(cJSON_GetObjectItem(data->get(), c.builtin().attrs(i).c_str()));
                    if (!item) {
                        throw runtime_exc_t("Attempting to pick missing attribute.");
                    } else {
                        cJSON_AddItemToObject(res->get(), item->string, item);
                    }
                }
                return res;
            }
            break;
        case Builtin::MAPMERGE:
            {
                boost::shared_ptr<scoped_cJSON_t> left  = eval(c.args(0), env),
                                                  right = eval(c.args(1), env);
                if (left->get()->type != cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
                }

                if (right->get()->type != cJSON_Object) {
                    throw runtime_exc_t("Data must be an object");
                }

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_DeepCopy(left->get())));

                // Extend with the right side (and overwrite if necessary)
                for(int i = 0; i < cJSON_GetArraySize(right->get()); i++) {
                    cJSON *item = cJSON_GetArrayItem(right->get(), i);
                    cJSON_DeleteItemFromObject(res->get(), item->string);
                    cJSON_AddItemToObject(res->get(), item->string, cJSON_DeepCopy(item));
                }

                return res;
            }
            break;
        case Builtin::ARRAYAPPEND:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array  = eval(c.args(0), env);
                if (array->get()->type != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_DeepCopy(array->get())));
                cJSON_AddItemToArray(res->get(), eval(c.args(1), env)->release());
                return res;
            }
            break;
        case Builtin::ARRAYCONCAT:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array1  = eval(c.args(0), env);
                if (array1->get()->type != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }
                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> array2  = eval(c.args(1), env);
                if (array2->get()->type != cJSON_Array) {
                    throw runtime_exc_t("The second argument must be an array.");
                }
                // Create new array and deep copy all the elements
                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));
                for(int i = 0; i < cJSON_GetArraySize(array1->get()); i++) {
                    cJSON_AddItemToArray(res->get(), cJSON_DeepCopy(cJSON_GetArrayItem(array1->get(), i)));
                }
                for(int j = 0; j < cJSON_GetArraySize(array2->get()); j++) {
                    cJSON_AddItemToArray(res->get(), cJSON_DeepCopy(cJSON_GetArrayItem(array2->get(), j)));
                }

                return res;
            }
            break;
        case Builtin::ARRAYSLICE:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array = eval(c.args(0), env);
                if (array->get()->type != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> start_json  = eval(c.args(1), env);
                if (start_json->get()->type != cJSON_Number) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }

                float float_start = start_json->get()->valuedouble;
                int start = (int)float_start;
                if (float_start != start) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }

                // Check third arg type
                boost::shared_ptr<scoped_cJSON_t> end_json  = eval(c.args(2), env);
                if (end_json->get()->type != cJSON_Number) {
                    throw runtime_exc_t("The third argument must be an integer.");
                }
                float float_end = end_json->get()->valuedouble;
                int stop = (int)float_end;
                if (float_end != stop) {
                    throw runtime_exc_t("The third argument must be an integer.");
                }

                int length = cJSON_GetArraySize(array->get());

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
                    cJSON_AddItemToArray(res->get(), cJSON_DeepCopy(cJSON_GetArrayItem(array->get(), i)));
                }

                return res;
            }
            break;
        case Builtin::ARRAYNTH:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array  = eval(c.args(0), env);
                if (array->get()->type != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> index_json  = eval(c.args(1), env);
                if (index_json->get()->type != cJSON_Number) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }
                float float_index = index_json->get()->valuedouble;
                int index = (int)float_index;
                if (float_index != index) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }

                int length = cJSON_GetArraySize(array->get());

                if (index < 0) {
                    index += length;
                }
                if (index < 0 || index >= length) {
                    throw runtime_exc_t("Array index out of bounds.");
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_DeepCopy(cJSON_GetArrayItem(array->get(),
                                                                                                              index))));
            }
            break;
        case Builtin::ARRAYLENGTH:
            {
                // Check first arg type
                boost::shared_ptr<scoped_cJSON_t> array  = eval(c.args(0), env);
                if (array->get()->type != cJSON_Array) {
                    throw runtime_exc_t("The first argument must be an array.");
                }
                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(cJSON_GetArraySize(array->get()))));
            }
            break;
        case Builtin::ADD:
            {
                double result = 0.0;

                for (int i = 0; i < c.args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                    if (arg->get()->type != cJSON_Number) {
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
                    if (arg->get()->type != cJSON_Number) {
                        throw runtime_exc_t("All operands to SUBTRACT must be numbers.");
                    }
                    if (c.args_size() == 1) {
                        result = -arg->get()->valuedouble;  // (- x) is negate
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c.args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                        if (arg->get()->type != cJSON_Number) {
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
                    if (arg->get()->type != cJSON_Number) {
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
                    if (arg->get()->type != cJSON_Number) {
                        throw runtime_exc_t("All operands to SUBTRACT must be numbers.");
                    }
                    if (c.args_size() == 1) {
                        result = 1.0 / arg->get()->valuedouble;  // (/ x) is reciprocal
                    } else {
                        result = arg->get()->valuedouble;
                    }

                    for (int i = 1; i < c.args_size(); ++i) {
                        boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                        if (arg->get()->type != cJSON_Number) {
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
                if (lhs->get()->type != cJSON_Number || rhs->get()->type != cJSON_Number) {
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

                int type = lhs->get()->type;

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
                        if (rhs->get()->type != type) {
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
                        if (rhs->get()->type != type) {
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
                        if (rhs->get()->type != cJSON_True && rhs->get()->type != cJSON_False) {
                            throw runtime_exc_t("Cannot compare these types.");
                        }

                        int lefttype = lhs->get()->type;
                        int righttype = rhs->get()->type;

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
        case Builtin::JAVASCRIPTRETURNINGSTREAM:
        case Builtin::UNION:
        case Builtin::RANGE:
            unreachable("eval called on a function that returns a stream (use eval_stream instead).\n");
            break;
        case Builtin::LENGTH:
            {
                throw runtime_exc_t("length not implemented\n");
                //json_stream_t stream = eval_stream(c.args(0), env);
                //return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(cJSON_CreateNumber(stream.size())));
            }
            break;
        case Builtin::NTH:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> index_json  = eval(c.args(1), env);
                if (index_json->get()->type != cJSON_Number) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }
                float index_float = index_json->get()->valuedouble;
                int index = (int)index_float;
                if (index_float != index || index < 0) {
                    throw runtime_exc_t("The second argument must be a nonnegative integer.");
                }

                boost::shared_ptr<scoped_cJSON_t> json;
                for (int i = 0; i < index; ++i) {
                    json = stream->next();
                    if (!json) {
                        throw runtime_exc_t("Index out of bounds.");
                    }
                }

                return boost::shared_ptr<scoped_cJSON_t>(new scoped_cJSON_t(
                    cJSON_DeepCopy(json->get())
                ));
            }
            break;
        case Builtin::STREAMTOARRAY:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                boost::shared_ptr<scoped_cJSON_t> res(new scoped_cJSON_t(cJSON_CreateArray()));

                while (boost::shared_ptr<scoped_cJSON_t> json = stream->next()) {
                    cJSON_AddItemToArray(res->get(), cJSON_DeepCopy(json->get()));
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
        case Builtin::GROUPEDMAPREDUCE:
            throw runtime_exc_t("Unimplemented: Builtin::GROUPEDMAPREDUCE");
            break;
        case Builtin::JAVASCRIPT:
            throw runtime_exc_t("Unimplemented: Builtin::JAVASCRIPT");
            break;
        case Builtin::MAPREDUCE:
            throw runtime_exc_t("Unimplemented: Builtin::MAPREDUCE");
            break;
        case Builtin::ALL:
            {
                bool result = true;

                for (int i = 0; i < c.args_size(); ++i) {
                    boost::shared_ptr<scoped_cJSON_t> arg = eval(c.args(i), env);
                    if (arg->get()->type != cJSON_False && arg->get()->type != cJSON_True) {
                        throw runtime_exc_t("All operands to ALL must be booleans.");
                    }
                    if (arg->get()->type != cJSON_True) {
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
                    if (arg->get()->type != cJSON_False && arg->get()->type != cJSON_True) {
                        throw runtime_exc_t("All operands to ANY must be booleans.");
                    }
                    if (arg->get()->type == cJSON_True) {
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

        if (a_bool->get()->type == cJSON_True) {
            return true;
        } else if (a_bool->get()->type == cJSON_False) {
            return false;
        } else {
            throw runtime_exc_t("Predicate failed to evaluate to a bool\n");
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

bool less(cJSON *l, cJSON *r) {
    switch (l->type) {
        case cJSON_False:
            if (r->type == cJSON_True) {
                return true;
            } else if (r->type == cJSON_False) {
                return false;
            } else {
                throw runtime_exc_t("Comparing boolean to non boolean\n");
            }
            break;
        case cJSON_True:
            if (r->type == cJSON_True) {
                return false;
            } else if (r->type == cJSON_False) {
                return false;
            } else {
                throw runtime_exc_t("Comparing boolean to non boolean\n");
            }
            break;
        case cJSON_NULL:
            throw runtime_exc_t("Can't compare null to anything\n");
            break;
        case cJSON_Number:
            if (r->type == cJSON_Number) {
                return l->valuedouble < r->valuedouble;
            } else {
                throw runtime_exc_t("Numbers can only be compared to other numbers.");
            }
            break;
        case cJSON_String:
            if (r->type == cJSON_String) {
                return (strcmp(l->valuestring, r->valuestring) < 0);
            } else {
                throw runtime_exc_t("Strings can only be compared to other strings.");
            }
            break;
        case cJSON_Array:
            throw runtime_exc_t("Can't compare arrays.");
            break;
        case cJSON_Object:
            throw runtime_exc_t("Can't compare objects.");
            break;
        default:
            unreachable();
            break;
    }
}

class ordering_t {
public:
    ordering_t(const Mapping &_mapping,  runtime_environment_t *_env)
        : mapping(_mapping), env(_env)
    { }

    //returns true of x < y
    bool operator()(boost::shared_ptr<scoped_cJSON_t> x, boost::shared_ptr<scoped_cJSON_t> y) {
        boost::shared_ptr<scoped_cJSON_t> x_mapped, y_mapped;
        {
            variable_val_scope_t::new_scope_t scope_maker(&env->scope);
            env->scope.put_in_scope(mapping.arg(), x);
            x_mapped = eval(mapping.body(), env);
        }

        {
            variable_val_scope_t::new_scope_t scope_maker(&env->scope);
            env->scope.put_in_scope(mapping.arg(), y);
            y_mapped = eval(mapping.body(), env);
        }

        return less(x_mapped->get(), y_mapped->get());
    }

private:
    Mapping mapping;
    runtime_environment_t *env;
};

struct shared_scoped_less {
    bool operator()(const boost::shared_ptr<scoped_cJSON_t> &a,
                      const boost::shared_ptr<scoped_cJSON_t> &b) {
        if (a->get()->type == b->get()->type) {
            return less(a->get(), b->get());
        } else {
            return a->get()->type > b->get()->type;
        }
    }
};

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
        case Builtin::GROUPEDMAPREDUCE:
        case Builtin::JAVASCRIPT:
        case Builtin::MAPREDUCE:
        case Builtin::ALL:
        case Builtin::ANY:
            unreachable("eval_stream called on a function that does not return a stream (use eval instead).\n");
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
                throw runtime_exc_t("Unimplemented: Builtin::MAP");
                //json_stream_t res;

                //for (json_stream_t::iterator it  = stream.begin();
                //                             it != stream.end();
                //                             ++it) {
                //    variable_val_scope_t::new_scope_t scope_maker(&env->scope);
                //    env->scope.put_in_scope(c.builtin().map().mapping().arg(), *it);
                //    res.push_back(eval(c.builtin().map().mapping().body(), env));
                //}
                //return res;
            }
            break;
        case Builtin::CONCATMAP:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);
                throw runtime_exc_t("Unimplemented: Builtin::CONCATMAP");
                //json_stream_t res;
            }
            break;
        case Builtin::ORDERBY:
            {
                ordering_t o(c.builtin().order_by().mapping(), env);
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);
                throw runtime_exc_t("Unimplemented: Builtin::ORDERBY");
                //stream.sort(o);
                //return stream;
            }
            break;
        case Builtin::DISTINCT:
            {
                std::set<boost::shared_ptr<scoped_cJSON_t>, shared_scoped_less> seen;

                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                throw runtime_exc_t("unimplemented distinct");
                //json_stream_t res;

                //for (json_stream_t::iterator it  = stream.begin();
                //                             it != stream.end();
                //                             ++it) {
                //    if (seen.find(*it) == seen.end()) {
                //        res.push_back(*it);
                //        seen.insert(*it);
                //    }
                //}

                //return res;
            }
            break;
        case Builtin::LIMIT:
            {
                boost::shared_ptr<json_stream_t> stream = eval_stream(c.args(0), env);

                // Check second arg type
                boost::shared_ptr<scoped_cJSON_t> limit_json  = eval(c.args(1), env);
                if (limit_json->get()->type != cJSON_Number) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }
                float limit_float = limit_json->get()->valuedouble;
                int limit = (int)limit_float;
                if (limit_float != limit) {
                    throw runtime_exc_t("The second argument must be an integer.");
                }

                throw runtime_exc_t("Unimplemented: Builtin::LIMIT");
                //if (int(stream.size()) > limit) {
                //    json_stream_t::iterator it = stream.begin();
                //    for (int i = 0; i < limit; ++i) {
                //        ++it;
                //    }
                //    stream.erase(it, stream.end());
                //}

                //return stream;
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
        case Builtin::JAVASCRIPTRETURNINGSTREAM:
            throw runtime_exc_t("Unimplemented: Builtin::JAVASCRIPTRETURNINGSTREAM");
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

/*
view_t eval(const Term &v, runtime_environment_t *env) {
    switch (v.type()) {
<<<<<<< HEAD
        case View::TABLE:
            {
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(v.table().table_ref(), env);
                key_range_t range = rdb_protocol_t::region_t::universe();
                rdb_protocol_t::rget_read_t rget_read(range);
                rdb_protocol_t::read_t read(rget_read);
                rdb_protocol_t::read_response_t res = ns_access.get_namespace_if()->read(read, order_token_t::ignore, &env->interruptor);
                rdb_protocol_t::rget_read_response_t *p_res = boost::get<rdb_protocol_t::rget_read_response_t>(&res.response);
                rassert(p_res);
                boost::shared_ptr<json_stream_t> stream;//(p_res->data.begin(), p_res->data.end());
                return view_t(ns_access, stream);
            }
            break;
||||||| merged common ancestors
        case View::TABLE:
            {
                namespace_repo_t<rdb_protocol_t>::access_t ns_access = eval(v.table().table_ref(), env);
                key_range_t range = rdb_protocol_t::region_t::universe();
                rdb_protocol_t::rget_read_t rget_read(range);
                rdb_protocol_t::read_t read(rget_read);
                rdb_protocol_t::read_response_t res = ns_access.get_namespace_if()->read(read, order_token_t::ignore, &env->interruptor);
                rdb_protocol_t::rget_read_response_t *p_res = boost::get<rdb_protocol_t::rget_read_response_t>(&res.response);
                rassert(p_res);
                json_stream_t stream(p_res->data.begin(), p_res->data.end());
                return view_t(ns_access, stream);
            }
            break;
=======
>>>>>>> d75e165ac28d5d867dae49b972b611ae981bbcc8
        case View::FILTERVIEW:
            {
                view_t subview = eval(v.filter_view().view(), env);

                predicate_t p(v.filter_view().predicate(), env);
<<<<<<< HEAD
                throw runtime_exc_t("Unimplemented: Builtin::FILTERVIEW");
                //subview.stream.remove_if(p);
                //return subview;
||||||| merged common ancestors
                subview.stream.remove_if(p);
                return subview;
=======
                subview.stream.remove_if(not_t(p));
                return subview;
>>>>>>> d75e165ac28d5d867dae49b972b611ae981bbcc8
            }
            break;
        case View::RANGEVIEW:
            throw runtime_exc_t("Unimplemented: View::RANGEVIEW");
            break;
        default:
            unreachable();
            break;
    }
}
*/

} //namespace query_language
