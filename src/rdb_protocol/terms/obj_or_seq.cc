#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pathspec.hpp"
#include "rdb_protocol/pb_utils.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

// This term is used for functions that are polymorphic on objects and
// sequences, like `pluck`.  It will handle the polymorphism; terms inheriting
// from it just need to implement evaluation on objects (`obj_eval`).
class obj_or_seq_op_term_t : public op_term_t {
public:
    enum poly_type_t {
        MAP = 0,
        FILTER = 1,
        SKIP_MAP = 2
    };
    obj_or_seq_op_term_t(env_t *env, protob_t<const Term> term,
                         poly_type_t _poly_type, argspec_t argspec)
        : op_term_t(env, term, argspec, optargspec_t({"_NO_RECURSE_"})),
          poly_type(_poly_type), func(make_counted_term()) {
        int varnum = env->gensym();
        Term *arg = pb::set_func(func.get(), varnum);
        Term *body = NULL;
        switch (poly_type) {
        case MAP: {
            body = arg;
            *arg = *term;
            Term_AssocPair *ap = arg->add_optargs();
            ap->set_key("_NO_RECURSE_");
            arg = ap->mutable_val();
            NDATUM_BOOL(true);
        } break;
        case FILTER: {
            body = arg;
            *arg = *term;
            Term_AssocPair *ap = arg->add_optargs();
            ap->set_key("_NO_RECURSE_");
            arg = ap->mutable_val();
            NDATUM_BOOL(true);
        } break;
        case SKIP_MAP: {
            N2(DEFAULT,
               N1(MAKE_ARRAY, body = arg; *arg = *term;
               Term_AssocPair *ap = arg->add_optargs();
               ap->set_key("_NO_RECURSE_");
               arg = ap->mutable_val();
               NDATUM_BOOL(true)),
               N0(MAKE_ARRAY));
        } break;
        default: unreachable();
        }
        r_sanity_check(body != NULL);
        pb::set_var(pb::reset(body->mutable_args(0)), varnum);
        prop_bt(func.get());
    }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) = 0;
    virtual counted_t<val_t> eval_impl() {
        counted_t<val_t> v0 = arg(0);
        counted_t<const datum_t> d;

        if (v0->get_type().is_convertible(val_t::type_t::DATUM)) {
            d = v0->as_datum();
        }

        if (d.has() && d->get_type() == datum_t::R_OBJECT) {
            return obj_eval(v0);
        } else if ((d.has() && d->get_type() == datum_t::R_ARRAY) ||
                   (!d.has() && v0->get_type().is_convertible(val_t::type_t::SEQUENCE))) {
            // The above if statement is complicated because it produces better
            // error messages on e.g. strings.
            if (counted_t<val_t> no_recurse = optarg("_NO_RECURSE_")) {
                rcheck(no_recurse->as_bool() == false, base_exc_t::GENERIC,
                       strprintf("Cannot perform %s on a sequence of sequences.", name()));
            }
            switch (poly_type) {
            case MAP:
                return new_val(v0->as_seq()->map(make_counted<func_t>(env, func)));
            case FILTER:
                return new_val(v0->as_seq()->filter(make_counted<func_t>(env, func)));
            case SKIP_MAP:
                return new_val(v0->as_seq()->concatmap(make_counted<func_t>(env, func)));
            default: unreachable();
            }
            unreachable();
        }

        rfail_typed_target(
            v0, "Cannot perform %s on a non-object non-sequence `%s`.",
            name(), v0->trunc_print().c_str());
        unreachable();
    }

    poly_type_t poly_type;
    protob_t<Term> func;
};

class pluck_term_t : public obj_or_seq_op_term_t {
public:
    pluck_term_t(env_t *env, protob_t<const Term> term) :
        obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) {
        counted_t<const datum_t> obj = v0->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        std::vector<counted_t<const datum_t> > paths;
        for (size_t i = 1; i < num_args(); ++i) {
            paths.push_back(arg(i)->as_datum());
        }
        pathspec_t pathspec(make_counted<const datum_t>(paths), this);
        return new_val(project(obj, pathspec, DONT_RECURSE));
    }
    virtual const char *name() const { return "pluck"; }
};

class without_term_t : public obj_or_seq_op_term_t {
public:
    without_term_t(env_t *env, protob_t<const Term> term) :
        obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) {
        counted_t<const datum_t> obj = v0->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        std::vector<counted_t<const datum_t> > paths;
        for (size_t i = 1; i < num_args(); ++i) {
            paths.push_back(arg(i)->as_datum());
        }
        pathspec_t pathspec(make_counted<const datum_t>(paths), this);
        return new_val(unproject(obj, pathspec, DONT_RECURSE));
    }
    virtual const char *name() const { return "without"; }
};

class merge_term_t : public obj_or_seq_op_term_t {
public:
    merge_term_t(env_t *env, protob_t<const Term> term) :
        obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) {
        counted_t<const datum_t> d = v0->as_datum();
        for (size_t i = 1; i < num_args(); ++i) {
            d = d->merge(arg(i)->as_datum());
        }
        return new_val(d);
    }
    virtual const char *name() const { return "merge"; }
};

class has_fields_term_t : public obj_or_seq_op_term_t {
public:
    has_fields_term_t(env_t *env, protob_t<const Term> term)
        : obj_or_seq_op_term_t(env, term, FILTER, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) {
        counted_t<const datum_t> obj = v0->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        std::vector<counted_t<const datum_t> > paths;
        for (size_t i = 1; i < num_args(); ++i) {
            paths.push_back(arg(i)->as_datum());
        }
        pathspec_t pathspec(make_counted<const datum_t>(paths), this);
        return new_val_bool(contains(obj, pathspec));
    }
    virtual const char *name() const { return "has_fields"; }
};

class get_field_term_t : public obj_or_seq_op_term_t {
public:
    get_field_term_t(env_t *env, protob_t<const Term> term)
        : obj_or_seq_op_term_t(env, term, SKIP_MAP, argspec_t(2)) { }
private:
    virtual counted_t<val_t> obj_eval(counted_t<val_t> v0) {
        return new_val(v0->as_datum()->get(arg(1)->as_str()));
    }
    virtual const char *name() const { return "get_field"; }
};

counted_t<term_t> make_get_field_term(env_t *env, protob_t<const Term> term) {
    return make_counted<get_field_term_t>(env, term);
}

counted_t<term_t> make_has_fields_term(env_t *env, protob_t<const Term> term) {
    return make_counted<has_fields_term_t>(env, term);
}

counted_t<term_t> make_pluck_term(env_t *env, protob_t<const Term> term) {
    return make_counted<pluck_term_t>(env, term);
}
counted_t<term_t> make_without_term(env_t *env, protob_t<const Term> term) {
    return make_counted<without_term_t>(env, term);
}
counted_t<term_t> make_merge_term(env_t *env, protob_t<const Term> term) {
    return make_counted<merge_term_t>(env, term);
}

} // namespace ql
