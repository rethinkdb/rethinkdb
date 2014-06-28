// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pathspec.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/pseudo_literal.hpp"
#include "rdb_protocol/minidriver.hpp"

namespace ql {

// This term is used for functions that are polymorphic on objects and
// sequences, like `pluck`.  It will handle the polymorphism; terms inheriting
// from it just need to implement evaluation on objects (`obj_eval`).
class obj_or_seq_op_term_t : public grouped_seq_op_term_t {
public:
    enum poly_type_t {
        MAP = 0,
        FILTER = 1,
        SKIP_MAP = 2
    };
    obj_or_seq_op_term_t(compile_env_t *env, protob_t<const Term> term,
                         poly_type_t _poly_type, argspec_t argspec)
        : grouped_seq_op_term_t(env, term, argspec, optargspec_t({"_NO_RECURSE_"})),
          poly_type(_poly_type), func(make_counted_term()) {

        auto varnum = pb::dummy_var_t::OBJORSEQ_VARNUM;

        // body is a new reql expression similar to term except that the first argument
        // is replaced by a new variable.
        // For example, foo.pluck('a') becomes varnum.pluck('a')
        r::reql_t body = r::var(varnum).call(term->type());
        body.copy_args_from_term(*term, 1);
        body.add_arg(r::optarg("_NO_RECURSE_", r::boolean(true)));

        switch (poly_type) {
        case MAP: {
            func->Swap(&r::fun(varnum, std::move(body)).get());
        } break;
        case FILTER: {
            func->Swap(&r::fun(varnum, std::move(body)).get());
        } break;
        case SKIP_MAP: {
            func->Swap(&r::fun(varnum,
                               r::array(std::move(body)).default_(r::array())).get());
        } break;
        default: unreachable();
        }

        prop_bt(func.get());
    }
private:
    virtual counted_t<val_t> obj_eval(scope_env_t *env,
                                      args_t *args,
                                      counted_t<val_t> v0) const = 0;

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        counted_t<val_t> v0 = args->arg(env, 0);
        counted_t<const datum_t> d;

        if (v0->get_type().is_convertible(val_t::type_t::DATUM)) {
            d = v0->as_datum();
        }

        if (d.has() && d->get_type() == datum_t::R_OBJECT) {
            return obj_eval(env, args, v0);
        } else if ((d.has() && d->get_type() == datum_t::R_ARRAY) ||
                   (!d.has()
                    && v0->get_type().is_convertible(val_t::type_t::SEQUENCE))) {
            // The above if statement is complicated because it produces better
            // error messages on e.g. strings.
            if (counted_t<val_t> no_recurse = args->optarg(env, "_NO_RECURSE_")) {
                rcheck(no_recurse->as_bool() == false, base_exc_t::GENERIC,
                       strprintf("Cannot perform %s on a sequence of sequences.",
                                 name()));
            }

            compile_env_t compile_env(env->scope.compute_visibility());
            counted_t<func_term_t> func_term
                = make_counted<func_term_t>(&compile_env, func);
            counted_t<func_t> f = func_term->eval_to_func(env->scope);

            counted_t<datum_stream_t> stream = v0->as_seq(env->env);
            switch (poly_type) {
            case MAP:
                stream->add_transformation(map_wire_func_t(f), backtrace());
                break;
            case FILTER:
                stream->add_transformation(filter_wire_func_t(f, boost::none),
                                           backtrace());
                break;
            case SKIP_MAP:
                stream->add_transformation(concatmap_wire_func_t(f),
                                           backtrace());
                break;
            default: unreachable();
            }

            return new_val(env->env, stream);
        }

        rfail_typed_target(
            v0, "Cannot perform %s on a non-object non-sequence `%s`.",
            name(), v0->trunc_print().c_str());
    }

    poly_type_t poly_type;
    protob_t<Term> func;
};

class pluck_term_t : public obj_or_seq_op_term_t {
public:
    pluck_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(scope_env_t *env, args_t *args, counted_t<val_t> v0) const {
        counted_t<const datum_t> obj = v0->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        const size_t n = args->num_args();
        std::vector<counted_t<const datum_t> > paths;
        paths.reserve(n - 1);
        for (size_t i = 1; i < n; ++i) {
            paths.push_back(args->arg(env, i)->as_datum());
        }
        pathspec_t pathspec(make_counted<const datum_t>(std::move(paths)), this);
        return new_val(project(obj, pathspec, DONT_RECURSE));
    }
    virtual const char *name() const { return "pluck"; }
};

class without_term_t : public obj_or_seq_op_term_t {
public:
    without_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(scope_env_t *env, args_t *args, counted_t<val_t> v0) const {
        counted_t<const datum_t> obj = v0->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        std::vector<counted_t<const datum_t> > paths;
        const size_t n = args->num_args();
        paths.reserve(n - 1);
        for (size_t i = 1; i < n; ++i) {
            paths.push_back(args->arg(env, i)->as_datum());
        }
        pathspec_t pathspec(make_counted<const datum_t>(std::move(paths)), this);
        return new_val(unproject(obj, pathspec, DONT_RECURSE));
    }
    virtual const char *name() const { return "without"; }
};

class literal_term_t : public op_term_t {
public:
    literal_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(0, 1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t flags) const {
        rcheck(flags & LITERAL_OK, base_exc_t::GENERIC,
               "Stray literal keyword found, literal can only be present inside merge "
               "and cannot nest inside other literals.");
        datum_ptr_t res(datum_t::R_OBJECT);
        bool clobber = res.add(datum_t::reql_type_string,
                               make_counted<const datum_t>(pseudo::literal_string));
        if (args->num_args() == 1) {
            clobber |= res.add(pseudo::value_key, args->arg(env, 0)->as_datum());
        }

        r_sanity_check(!clobber);
        std::set<std::string> permissible_ptypes;
        permissible_ptypes.insert(pseudo::literal_string);
        return new_val(res.to_counted(permissible_ptypes));
    }
    virtual const char *name() const { return "literal"; }
    virtual bool can_be_grouped() const { return false; }
};

class merge_term_t : public obj_or_seq_op_term_t {
public:
    merge_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1, LITERAL_OK)) { }
private:
    virtual counted_t<val_t> obj_eval(scope_env_t *env, args_t *args, counted_t<val_t> v0) const {
        counted_t<const datum_t> d = v0->as_datum();
        for (size_t i = 1; i < args->num_args(); ++i) {
            counted_t<val_t> v = args->arg(env, i, LITERAL_OK);

            // We branch here because compiling functions is expensive, and
            // `obj_eval` may be called many many times.
            if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
                d = d->merge(v->as_datum());
            } else {
                auto f = v->as_func(CONSTANT_SHORTCUT);
                d = d->merge(f->call(env->env, d, LITERAL_OK)->as_datum());
            }
        }
        return new_val(d);
    }
    virtual const char *name() const { return "merge"; }
};

class has_fields_term_t : public obj_or_seq_op_term_t {
public:
    has_fields_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : obj_or_seq_op_term_t(env, term, FILTER, argspec_t(1, -1)) { }
private:
    virtual counted_t<val_t> obj_eval(scope_env_t *env, args_t *args, counted_t<val_t> v0) const {
        counted_t<const datum_t> obj = v0->as_datum();
        r_sanity_check(obj->get_type() == datum_t::R_OBJECT);

        std::vector<counted_t<const datum_t> > paths;
        const size_t n = args->num_args();
        paths.reserve(n - 1);
        for (size_t i = 1; i < n; ++i) {
            paths.push_back(args->arg(env, i)->as_datum());
        }
        pathspec_t pathspec(make_counted<const datum_t>(std::move(paths)), this);
        return new_val_bool(contains(obj, pathspec));
    }
    virtual const char *name() const { return "has_fields"; }
};

class get_field_term_t : public obj_or_seq_op_term_t {
public:
    get_field_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : obj_or_seq_op_term_t(env, term, SKIP_MAP, argspec_t(2)) { }
private:
    virtual counted_t<val_t> obj_eval(scope_env_t *env, args_t *args, counted_t<val_t> v0) const {
        return new_val(v0->as_datum()->get(args->arg(env, 1)->as_str().to_std()));
    }
    virtual const char *name() const { return "get_field"; }
};

counted_t<term_t> make_get_field_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<get_field_term_t>(env, term);
}

counted_t<term_t> make_has_fields_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<has_fields_term_t>(env, term);
}

counted_t<term_t> make_pluck_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<pluck_term_t>(env, term);
}
counted_t<term_t> make_without_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<without_term_t>(env, term);
}
counted_t<term_t> make_literal_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<literal_term_t>(env, term);
}
counted_t<term_t> make_merge_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<merge_term_t>(env, term);
}

} // namespace ql
