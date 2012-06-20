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
            CHECK_WELL_DEFINED(t.get_by_key().value());
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
