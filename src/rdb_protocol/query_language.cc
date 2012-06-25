#include "rdb_protocol/query_language.hpp"

#include "errors.hpp"

#define CHECK_WELL_DEFINED(x) if (! is_well_defined(x)) { return false; }

bool is_well_defined(const VarTermTuple &v) {
    return is_well_defined(v.term());
}

bool is_well_defined(const Term &t) {
    rassert(t.has_type());
    rassert(Term_TermType_IsValid(t.type()));

    if (t.has_var()) {
        if (t.type() != Term::VAR) {
            return false;
        } else {
            //no other way for this to fail
        }
    }

    if (t.has_let()) {
        if (t.type() != Term::LET) {
            return false;
        } else {
            for (int i = 0; i < t.let().binds_size(); ++i) {
                CHECK_WELL_DEFINED(t.let().binds(i));
            }
        }
        CHECK_WELL_DEFINED(t.let().expr());
    }

    if (t.has_call()) {
        if (t.type() != Term::CALL) {
            return false;
        } else {
            CHECK_WELL_DEFINED(t.call().builtin());
            for (int i = 0; i < t.call().args_size(); ++i) {
                CHECK_WELL_DEFINED(t.call().args(i));
            }
        }
    }

    if (t.has_if_()) {
        if (t.type() != Term::IF) {
            return false;
        } else {
            CHECK_WELL_DEFINED(t.if_().test());
            CHECK_WELL_DEFINED(t.if_().true_branch());
            CHECK_WELL_DEFINED(t.if_().false_branch());
        }
    }

    if (t.has_try_()) {
        if (t.type() != Term::TRY) {
            return false;
        } else {
            CHECK_WELL_DEFINED(t.try_().try_term());
            CHECK_WELL_DEFINED(t.try_().var_and_catch_term().term());
        }
    }

    if (t.has_error() && t.type() != Term::ERROR) {
        return false;
    }

    if (t.has_number() && t.type() != Term::NUMBER) {
        return false;
    }

    if (t.has_valuestring() && t.type() != Term::STRING) {
        return false;
    }

    if (t.has_valuebool() && t.type() != Term::BOOL) {
        return false;
    }

    if (t.array_size() > 0 && t.type() != Term::ARRAY) {
        return false;
    }

    if (t.map_size() > 0 && t.type() != Term::MAP) {
        return false;
    }

    if (t.has_view_as_stream() && t.type() != Term::VIEWASSTREAM) {
        return false;
    }

    if (t.has_get_by_key()) {
        if (t.type() != Term::GETBYKEY) {
            return false;
        } else {
            CHECK_WELL_DEFINED(t.get_by_key().key());
        }
    }

    return true;
}

bool is_well_defined(const Builtin &b) {
    rassert(b.has_type());
    rassert(Builtin_BuiltinType_IsValid(b.type()));

    if (b.has_attr() && b.type() != Builtin::GETATTR && b.type() != Builtin::HASATTR) {
        return false;
    }

    if (b.attrs_size() > 0 && b.type() != Builtin::PICKATTRS) {
        return false;
    }

    if (b.has_filter()) {
        if (b.type() != Builtin::FILTER) {
            return false;
        } else {
            CHECK_WELL_DEFINED(b.filter().predicate());
        }
    }

    if (b.has_map()) {
        if (b.type() != Builtin::MAP) {
            return false;
        } else {
            CHECK_WELL_DEFINED(b.map().mapping());
        }
    }

    if (b.has_concat_map()) {
        if (b.type() != Builtin::CONCATMAP) {
            return false;
        } else {
            CHECK_WELL_DEFINED(b.concat_map().mapping());
        }
    }

    if (b.has_order_by()) {
        if (b.type() != Builtin::ORDERBY) {
            return false;
        } else {
            CHECK_WELL_DEFINED(b.order_by().mapping());
        }
    }

    if (b.has_distinct()) {
        if (b.type() != Builtin::DISTINCT) {
            return false;
        } else {
            CHECK_WELL_DEFINED(b.distinct().mapping());
        }
    }

    if (b.has_limit() && b.type() != Builtin::LIMIT) {
        return false;
    }

    if (b.has_reduce()) {
        if (b.type() != Builtin::REDUCE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(b.reduce().reduction());
        }
    }

    if (b.has_grouped_map_reduce()) {
        if (b.type() != Builtin::GROUPEDMAPREDUCE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(b.grouped_map_reduce().group_mapping());
            CHECK_WELL_DEFINED(b.grouped_map_reduce().map_reduce().change_mapping());
            CHECK_WELL_DEFINED(b.grouped_map_reduce().map_reduce().reduction());
        }
    }

    if (b.has_map_reduce()) {
        if (b.type() != Builtin::MAPREDUCE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(b.map_reduce().change_mapping());
            CHECK_WELL_DEFINED(b.map_reduce().reduction());
        }
    }

    return true;
}

bool is_well_defined(const Reduction &r) {
    CHECK_WELL_DEFINED(r.base())
    CHECK_WELL_DEFINED(r.var1())
    CHECK_WELL_DEFINED(r.var2())
    CHECK_WELL_DEFINED(r.body())
    return true;
}

bool is_well_defined(const Mapping &m) {
    CHECK_WELL_DEFINED(m.arg());
    CHECK_WELL_DEFINED(m.body());
    return true;
}

bool is_well_defined(const Predicate &p) {
    CHECK_WELL_DEFINED(p.arg());
    CHECK_WELL_DEFINED(p.body());
    return true;
}

bool is_well_defined(const View &v) {
    if (v.has_table() && v.type() != View::TABLE) {
        return false;
    }

    if (v.has_filter_view()) {
        if (v.type() != View::FILTERVIEW) {
            return false;
        } else {
            CHECK_WELL_DEFINED(v.filter_view().view());
            CHECK_WELL_DEFINED(v.filter_view().predicate());
        }
    }

    if (v.has_range_view()) {
        if (v.type() != View::RANGEVIEW) {
            return false;
        } else {
            CHECK_WELL_DEFINED(v.range_view().view());
            CHECK_WELL_DEFINED(v.range_view().lowerbound());
            CHECK_WELL_DEFINED(v.range_view().upperbound());
        }
    }

    return true;
}

bool is_well_defined(const ReadQuery &r) {
    return is_well_defined(r.term());
}

bool is_well_defined(const WriteQuery &w) {
    rassert(w.has_type());
    rassert(WriteQuery_WriteQueryType_IsValid(w.type()));

    if (w.has_update()) {
        if (w.type() != WriteQuery::UPDATE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(w.update().view());
            CHECK_WELL_DEFINED(w.update().mapping());
        }
    }

    if (w.has_delete_()) {
        if (w.type() != WriteQuery::DELETE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(w.delete_().view());
        }
    }

    if (w.has_mutate()) {
        if (w.type() != WriteQuery::MUTATE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(w.mutate().view());
            CHECK_WELL_DEFINED(w.mutate().mapping());
        }
    }

    if (w.has_insert()) {
        if (w.type() != WriteQuery::INSERT) {
            return false;
        } else {
            for (int i = 0; i < w.insert().terms_size(); ++i) {
                CHECK_WELL_DEFINED(w.insert().terms(i));
            }
        }
    }

    if (w.has_insert_stream()) {
        if (w.type() != WriteQuery::INSERTSTREAM) {
            return false;
        } else {
            CHECK_WELL_DEFINED(w.insert_stream().stream());
        }
    }

    if (w.has_for_each()) {
        if (w.type() != WriteQuery::FOREACH) {
            return false;
        } else {
            CHECK_WELL_DEFINED(w.for_each().stream());
            CHECK_WELL_DEFINED(w.for_each().var());
            for (int i = 0; i < w.for_each().queries_size(); ++i) {
                CHECK_WELL_DEFINED(w.for_each().queries(i));
            }
        }
    }

    if (w.has_point_update()) {
        if (w.type() != WriteQuery::POINTUPDATE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(w.point_update().key());
            CHECK_WELL_DEFINED(w.point_update().mapping());
        }
    }

    if (w.has_point_delete()) {
        if (w.type() != WriteQuery::POINTDELETE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(w.point_delete().key());
        }
    }

    if (w.has_point_mutate()) {
        if (w.type() != WriteQuery::POINTMUTATE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(w.point_mutate().key());
        }
    }

    return true;
}

bool is_well_defined(const Query &q) {
    if (q.has_read_query()) {
        if (q.type() != Query::READ) {
            return false;
        } else {
            CHECK_WELL_DEFINED(q.read_query());
        }

        if (q.type() != Query::WRITE) {
            return false;
        } else {
            CHECK_WELL_DEFINED(q.write_query());
        }
    }

    return true;
}

namespace query_language {
type_t get_type(const Term &t, variable_type_scope_t *scope) {
    switch (t.type()) {
        case Term::VAR:
            return scope->get_type(t.var());
            break;
        case Term::LET:
            {
                scope->push(); //create a new scope
                for (int i = 0; i < t.let().binds_size(); ++i) {
                    scope->put_in_scope(t.let().binds(i).var().var(), get_type(t.let().binds(i).term(), scope));
                }
                type_t res = get_type(t.let().expr(), scope);
                scope->pop();
                return res;
                break;
            }
        case Term::CALL:
            {
                function_t signature = get_type(t.call().builtin(), scope);
                if (t.call().args_size() + 1 > int(signature.size())) {
                    return error_t(strprintf("Too many arguments passed to function. Expected %d but got %d", int(signature.size() - 1), t.call().args_size())); //TODO would be nice to have function names attached to errors
                } else if (t.call().args_size() + 1 < int(signature.size())) {
                    return error_t(strprintf("Too few  arguments passed to function. Expected %d but got %d", int(signature.size() - 1), t.call().args_size())); //TODO would be nice to have function names attached to errors
                }
                for (int i = 0; i < t.call().args_size(); ++i) {
                    if (get_type(t.call().args(i), scope) == signature.front()) {
                        rassert(!signature.empty());
                        signature.pop_front();
                    } else {
                        return error_t("Type mismatch in function call\n"); //Need descriptions of types to give a more informative message
                    }
                }
                rassert(signature.size() == 1);
                return signature.front();
                break;
            }
        case Term::IF:
            {
                if (!(get_type(t.if_().test(), scope) == JSON())) {
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
        case Term::TRY:
            {
                type_t try_type = get_type(t.try_().try_term(), scope);
                scope->push();
                scope->put_in_scope(t.try_().var_and_catch_term().var().var(), primitive_t(primitive_t::ERROR));
                type_t catch_type = get_type(t.try_().var_and_catch_term().term(), scope);
                scope->pop();
                if (!(try_type == catch_type)) {
                    return error_t("Mismatch between try and catch branch types.");
                } else {
                    return try_type;
                }
                break;
            }
        case Term::ERROR:
            return primitive_t(primitive_t::ERROR);
            break;
        case Term::NUMBER:
            return primitive_t(primitive_t::JSON);
            break;
        case Term::STRING:
            return primitive_t(primitive_t::JSON);
            break;
        case Term::JSON:
            return primitive_t(primitive_t::JSON);
            break;
        case Term::BOOL:
            return primitive_t(primitive_t::JSON);
            break;
        case Term::JSON_NULL:
            return primitive_t(primitive_t::JSON);
            break;
        case Term::ARRAY:
            return primitive_t(primitive_t::JSON);
            break;
        case Term::MAP:
            return primitive_t(primitive_t::JSON);
            break;
        case Term::VIEWASSTREAM:
            return primitive_t(primitive_t::STREAM);
            break;
        case Term::GETBYKEY:
            if (get_type(t.get_by_key().key(), scope) == JSON()) {
                return primitive_t(primitive_t::JSON);
            } else {
                return error_t("Key must be a json value.");
            }
            break;
        default:
            crash("unreachable");
            break;
    }
    crash("unreachable");
}

function_t get_type(const Builtin &b, variable_type_scope_t *) {
    function_t res;
    switch (b.type()) {
        //JSON -> JSON
        case Builtin::NOT:
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::GETATTR:
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::HASATTR:
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::PICKATTRS:
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::MAPMERGE:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::ARRAYCONS:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::ARRAYCONCAT:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::ARRAYSLICE:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::ARRAYNTH:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::ARRAYLENGTH:
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::ADD:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::SUBTRACT:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::MULTIPLY:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::DIVIDE:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::MODULO:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::COMPARE:
            res.push_back(JSON());
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::FILTER:
            res.push_back(STREAM());
            res.push_back(STREAM());
			break;
        case Builtin::MAP:
            res.push_back(STREAM());
            res.push_back(STREAM());
			break;
        case Builtin::CONCATMAP:
            res.push_back(STREAM());
            res.push_back(STREAM());
			break;
        case Builtin::ORDERBY:
            res.push_back(STREAM());
            res.push_back(STREAM());
			break;
        case Builtin::DISTINCT:
            res.push_back(STREAM());
            res.push_back(STREAM());
			break;
        case Builtin::LIMIT:
            res.push_back(STREAM());
            res.push_back(STREAM());
			break;
        case Builtin::LENGTH:
            res.push_back(STREAM());
            res.push_back(JSON());
			break;
        case Builtin::UNION:
            res.push_back(STREAM());
            res.push_back(STREAM());
            res.push_back(STREAM());
			break;
        case Builtin::NTH:
            res.push_back(STREAM());
            res.push_back(JSON());
			break;
        case Builtin::STREAMTOARRAY:
            res.push_back(STREAM());
            res.push_back(JSON());
			break;
        case Builtin::ARRAYTOSTREAM:
            res.push_back(JSON());
            res.push_back(STREAM());
			break;
        case Builtin::REDUCE:
            res.push_back(STREAM());
            res.push_back(JSON());
			break;
        case Builtin::GROUPEDMAPREDUCE:
            res.push_back(STREAM());
            res.push_back(JSON());
			break;
        case Builtin::JAVASCRIPT:
            res.push_back(JSON());
            res.push_back(JSON());
			break;
        case Builtin::JAVASCRIPTRETURNINGSTREAM:
            res.push_back(JSON());
            res.push_back(STREAM());
			break;
        case Builtin::MAPREDUCE:
            res.push_back(STREAM());
            res.push_back(JSON());
			break;
        default:
            crash("unreachable");
            break;
    }
    return res;
}

type_t get_type(const Reduction &r, variable_type_scope_t *scope) {
    if (!(get_type(r.base(), scope) == JSON())) {
        return type_t(error_t());
    }

    if (r.var1().type() != Term::VAR) {
        return type_t(error_t());
    }

    if (r.var2().type() != Term::VAR) {
        return type_t(error_t());
    }

    new_scope_t scope_maker(scope);
    scope->put_in_scope(r.var1().var(), JSON());
    scope->put_in_scope(r.var2().var(), JSON());

    if (!(get_type(r.body(), scope) == JSON())) {
        return type_t(error_t());
    } else {
        return JSON();
    }
}

type_t get_type(const Mapping &m, variable_type_scope_t *scope) {
    if (m.arg().type() != Term::VAR) {
        return type_t(error_t());
    }

    new_scope_t scope_maker(scope);
    scope->put_in_scope(m.arg().var(), JSON());

    if (!(get_type(m.body(), scope) == JSON())) {
        return type_t(error_t());
    } else {
        return JSON();
    }
}

type_t get_type(const Predicate &p, variable_type_scope_t *scope) {
    if (p.arg().type() != Term::VAR) {
        return type_t(error_t());
    }

    new_scope_t scope_maker(scope);
    scope->put_in_scope(p.arg().var(), JSON());

    if (!(get_type(p.body(), scope) == JSON())) {
        return type_t(error_t());
    } else {
        return JSON();
    }
}

type_t get_type(const View &v, variable_type_scope_t *scope) {
    switch(v.type()) {
        case View::TABLE:
            //no way for this to be incorrect
            return VIEW();
            break;
        case View::FILTERVIEW:
            if (get_type(v.filter_view().view(), scope) == VIEW() &&
                get_type(v.filter_view().predicate(), scope) == JSON()) {
                return VIEW();
            }
            break;
        case View::RANGEVIEW:
            if (get_type(v.range_view().view(), scope) == VIEW() &&
                get_type(v.range_view().lowerbound(), scope) == JSON() &&
                get_type(v.range_view().upperbound(), scope) == JSON()) {
                return READ();
            }
            break;
        default:
            crash("Unreachable");
    }
    crash("Unreachable");
}

type_t get_type(const ReadQuery &r, variable_type_scope_t *scope) {
    type_t res = get_type(r.term(), scope);
    if (res == JSON() ||
        res == STREAM()) {
        return READ();
    } else {
        return error_t("ReadQueries must produce either JSON or a STREAM.");
    }
}

type_t get_type(const WriteQuery &w, variable_type_scope_t *scope) {
    switch (w.type()) {
        case WriteQuery::UPDATE:
            if (get_type(w.update().view(), scope) == VIEW() &&
                get_type(w.update().mapping(), scope) == JSON()) {
                return WRITE();
            }
            break;
        case WriteQuery::DELETE:
            if (get_type(w.update().view(), scope) == VIEW()) {
                return WRITE();
            }
            break;
        case WriteQuery::MUTATE:
            if (get_type(w.mutate().view(), scope) == VIEW() &&
                get_type(w.mutate().mapping(), scope) == JSON()) {
                return WRITE();
            }
            break;
        case WriteQuery::INSERT:
            for (int i = 0; i < w.insert().terms_size(); ++i) {
                if (!(get_type(w.insert().terms(i), scope) == JSON())) {
                    return type_t(error_t("Trying to insert a non JSON term"));
                }
            }
            return WRITE();
            break;
        case WriteQuery::INSERTSTREAM:
            if (!(get_type(w.insert_stream().stream(), scope) == STREAM())) {
                return WRITE();
            }
            break;
        case WriteQuery::FOREACH:
            {
                if (!(get_type(w.for_each().stream(), scope) == STREAM())) {
                    return type_t(error_t("Must pass a stream in to a FOREACH query."));
                }

                new_scope_t scope_maker(scope);
                scope->put_in_scope(w.for_each().var().var(), JSON());
                for (int i = 0; i < w.for_each().queries_size(); ++i) {
                    if (!(get_type(w.for_each().queries(i), scope) == WRITE())) {
                        return type_t(error_t("Queries passed to a foreach must all be write queries\n"));
                    }
                }
                return WRITE();
            }
            break;
        case WriteQuery::POINTUPDATE:
            if (!(get_type(w.point_update().key(), scope) == JSON()) ||
                !(get_type(w.point_update().mapping(), scope) == JSON())) {
                return type_t(error_t("Key and mapping must both be of type JSON\n"));
            }
            return WRITE();
            break;
        case WriteQuery::POINTDELETE:
            if (!(get_type(w.point_delete().key(), scope) == JSON())) {
                return type_t(error_t("Key must be of type JSON\n"));
            }
            return WRITE();
            break;
        case WriteQuery::POINTMUTATE:
            if (!(get_type(w.point_mutate().key(), scope) == JSON()) ||
                !(get_type(w.point_mutate().mapping(), scope) == JSON())) {
                return type_t(error_t("Key and mapping must both be of type JSON\n"));
            }
            return WRITE();
            break;
        default:
            break;
    }
    crash("Unreachable");
}

type_t get_type(const Query &q, variable_type_scope_t *scope) {
    switch (q.type()) {
        case Query::READ:
            if (!(get_type(q.read_query(), scope) == READ())) {
                return type_t(error_t("Malformed read."));
            }
            return QUERY();
            break;
        case Query::WRITE:
            if (!(get_type(q.write_query(), scope) == WRITE())) {
                return type_t(error_t("Malformed write."));
            }
            return QUERY();
            break;
        default:
            crash("unreachable");
    }
    crash("unreachable");
}

} //namespace query_language
