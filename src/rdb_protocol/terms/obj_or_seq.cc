// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>
#include <functional>

#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pathspec.hpp"
#include "rdb_protocol/pseudo_literal.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rdb_protocol/terms/arr.hpp"
#include "rdb_protocol/terms/obj_or_seq.hpp"

namespace ql {

// Returns a new reql expression similar to term except that the first argument
// is replaced by a new variable.
// For example, foo.pluck('a') becomes varnum.pluck('a')
raw_term_t make_obj_or_seq_func(const raw_term_t &term,
                                poly_type_t poly_type) {
    minidriver_t r(term.bt());
    auto varnum = minidriver_t::dummy_var_t::OBJORSEQ_VARNUM;

    minidriver_t::reql_t body = r.var(varnum).call(term.type());
    body.copy_args_from_term(term, 1);
    body.add_arg(r.optarg("_NO_RECURSE_", r.boolean(true)));

    switch (poly_type) {
    case MAP: // fallthru
    case FILTER:
        return r.fun(varnum, body).root_term();
    case SKIP_MAP:
        return r.fun(varnum, r.array(body).default_(r.array())).root_term();
    default: unreachable();
    }
}

obj_or_seq_op_impl_t::obj_or_seq_op_impl_t(
        const term_t *_parent, poly_type_t _poly_type,
        std::set<std::string> &&_acceptable_ptypes)
    : parent(_parent), poly_type(_poly_type),
      func(make_obj_or_seq_func(parent->get_src(), poly_type)),
      acceptable_ptypes(std::move(_acceptable_ptypes)) { }

scoped_ptr_t<val_t> obj_or_seq_op_impl_t::eval_impl_dereferenced(
        eval_error *err_out, const term_t *target, scope_env_t *env, args_t *args,
        const scoped_ptr_t<val_t> &v0,
        std::function<scoped_ptr_t<val_t>(eval_error *)> helper) const {
    using noval = scoped_ptr_t<val_t>;
    datum_t d;

    if (v0->get_type().is_convertible(val_t::type_t::DATUM)) {
        d = v0->as_datum();
    }

    if (d.has() && d.get_type() == datum_t::R_OBJECT) {
        if (d.is_ptype() &&
            acceptable_ptypes.find(d.get_reql_type()) == acceptable_ptypes.end()) {
            rfail_target(v0, base_exc_t::LOGIC,
                         "Cannot call `%s` on objects of type `%s`.",
                         parent->name(), d.get_type_name().c_str());
        }
        return helper(err_out);
    } else if ((d.has() && d.get_type() == datum_t::R_ARRAY) ||
               (!d.has()
                && v0->get_type().is_convertible(val_t::type_t::SEQUENCE))) {
        // The above if statement is complicated because it produces better
        // error messages on e.g. strings.
        auto no_recurse = args->optarg(err_out, env, "_NO_RECURSE_");
        if (err_out->has()) { return noval(); }
        if (no_recurse) {
            rcheck_target(target,
                          no_recurse->as_bool() == false,
                          base_exc_t::LOGIC,
                          strprintf("Cannot perform %s on a sequence of sequences.",
                                    target->name()));
        }

        compile_env_t compile_env(env->scope.compute_visibility());
        counted_t<func_term_t> func_term =
            make_counted<func_term_t>(&compile_env, func);
        counted_t<const func_t> f =
            func_term->eval_to_func(env->scope);

        counted_t<datum_stream_t> stream;
        counted_t<selection_t> sel; // may be empty
        if (poly_type == FILTER
            && v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            sel = v0->as_selection(env->env);
            stream = sel->seq;
        } else {
            stream = v0->as_seq(env->env);
        }
        switch (poly_type) {
        case MAP:
            stream->add_transformation(map_wire_func_t(f), target->backtrace());
            break;
        case FILTER:
            stream->add_transformation(filter_wire_func_t(f, r_nullopt),
                                       target->backtrace());
            break;
        case SKIP_MAP:
            stream->add_transformation(
                concatmap_wire_func_t(result_hint_t::AT_MOST_ONE, f),
                target->backtrace());
            break;
        default: unreachable();
        }

        if (sel) {
            return target->new_val(sel);
        } else {
            return target->new_val(env->env, stream);
        }
    }

    rfail_typed_target(
        v0, "Cannot perform %s on a non-object non-sequence `%s`.",
        target->name(), v0->trunc_print().c_str());
}

obj_or_seq_op_term_t::obj_or_seq_op_term_t(
        compile_env_t *env, const raw_term_t &term,
        poly_type_t _poly_type, argspec_t argspec)
    : grouped_seq_op_term_t(env, term, argspec, optargspec_t({"_NO_RECURSE_"})),
      impl(this, _poly_type, std::set<std::string>()) {
}

obj_or_seq_op_term_t::obj_or_seq_op_term_t(
        compile_env_t *env, const raw_term_t &term,
        poly_type_t _poly_type, argspec_t argspec, std::set<std::string> &&ptypes)
    : grouped_seq_op_term_t(env, term, argspec, optargspec_t({"_NO_RECURSE_"})),
      impl(this, _poly_type, std::move(ptypes)) {
}

scoped_ptr_t<val_t> obj_or_seq_op_term_t::eval_impl(eval_error *err_out, scope_env_t *env, args_t *args,
                                                    eval_flags_t) const {
    scoped_ptr_t<val_t> v0 = args->arg(err_out, env, 0);
    if (err_out->has()) { return noval(); }
    return impl.eval_impl_dereferenced(err_out, this, env, args, v0,
        [&](eval_error *err_out_){ return this->obj_eval(err_out_, env, args, v0); });
}

class pluck_term_t : public obj_or_seq_op_term_t {
public:
    pluck_term_t(compile_env_t *env, const raw_term_t &term)
        : obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual scoped_ptr_t<val_t> obj_eval(
        eval_error *err_out, scope_env_t *env, args_t *args, const scoped_ptr_t<val_t> &v0) const {
        datum_t obj = v0->as_datum();
        r_sanity_check(obj.get_type() == datum_t::R_OBJECT);

        const size_t n = args->num_args();
        std::vector<datum_t> paths;
        paths.reserve(n - 1);
        for (size_t i = 1; i < n; ++i) {
            auto v_i = args->arg(err_out, env, i);
            if (err_out->has()) { return noval(); }
            paths.push_back(v_i->as_datum());
        }
        pathspec_t pathspec(datum_t(std::move(paths), env->env->limits()), this);
        return new_val(project(obj, pathspec, DONT_RECURSE, env->env->limits()));
    }
    virtual const char *name() const { return "pluck"; }
};

class without_term_t : public obj_or_seq_op_term_t {
public:
    without_term_t(compile_env_t *env, const raw_term_t &term)
        : obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1)) { }
private:
    virtual scoped_ptr_t<val_t> obj_eval(
        eval_error *err_out, scope_env_t *env, args_t *args, const scoped_ptr_t<val_t> &v0) const {
        datum_t obj = v0->as_datum();
        r_sanity_check(obj.get_type() == datum_t::R_OBJECT);

        std::vector<datum_t> paths;
        const size_t n = args->num_args();
        paths.reserve(n - 1);
        for (size_t i = 1; i < n; ++i) {
            auto v_i = args->arg(err_out, env, i);
            if (err_out->has()) { return noval(); }
            paths.push_back(v_i->as_datum());
        }
        pathspec_t pathspec(datum_t(std::move(paths), env->env->limits()), this);
        return new_val(unproject(obj, pathspec, DONT_RECURSE, env->env->limits()));
    }
    virtual const char *name() const { return "without"; }
};

class literal_term_t : public op_term_t {
public:
    literal_term_t(compile_env_t *env, const raw_term_t &term)
        : op_term_t(env, term, argspec_t(0, 1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(
        eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t flags) const {
        rcheck(flags & LITERAL_OK, base_exc_t::LOGIC,
               "Stray literal keyword found: literal is only legal inside of "
               "the object passed to merge or update and cannot nest inside "
               "other literals.");
        datum_object_builder_t res;
        bool clobber = res.add(datum_t::reql_type_string,
                               datum_t(pseudo::literal_string));
        if (args->num_args() == 1) {
            auto v0 = args->arg(err_out, env, 0);
            if (err_out->has()) { return noval(); }
            clobber |= res.add(pseudo::value_key, v0->as_datum());
        }

        r_sanity_check(!clobber);
        std::set<std::string> permissible_ptypes { pseudo::literal_string };
        return new_val(std::move(res).to_datum(permissible_ptypes));
    }
    virtual const char *name() const { return "literal"; }
    virtual bool can_be_grouped() const { return false; }
};

class merge_term_t : public obj_or_seq_op_term_t {
public:
    merge_term_t(compile_env_t *env, const raw_term_t &term)
        : obj_or_seq_op_term_t(env, term, MAP, argspec_t(1, -1, LITERAL_OK)) { }
private:
    virtual scoped_ptr_t<val_t> obj_eval(
        eval_error *err_out, scope_env_t *env, args_t *args, const scoped_ptr_t<val_t> &v0) const {
        datum_t d = v0->as_datum();
        for (size_t i = 1; i < args->num_args(); ++i) {
            scoped_ptr_t<val_t> v = args->arg(err_out, env, i, LITERAL_OK);
            if (err_out->has()) { return noval(); }

            // We branch here because compiling functions is expensive, and
            // `obj_eval` may be called many many times.
            if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
                datum_t d0 = v->as_datum();
                if (d0.get_type() == datum_t::R_OBJECT) {
                    rcheck_target(v,
                                  !d0.is_ptype() || d0.is_ptype("LITERAL"),
                                  base_exc_t::LOGIC,
                                  strprintf("Cannot merge objects of type `%s`.",
                                            d0.get_type_name().c_str()));
                }
                d = d.merge(d0);
            } else {
                auto f = v->as_func(CONSTANT_SHORTCUT);
                datum_t d0 = f->call(env->env, d, LITERAL_OK)->as_datum();
                if (d0.get_type() == datum_t::R_OBJECT) {
                    rcheck_target(v,
                                  !d0.is_ptype() || d0.is_ptype("LITERAL"),
                                  base_exc_t::LOGIC,
                                  strprintf("Cannot merge objects of type `%s`.",
                                            d0.get_type_name().c_str()));
                }
                d = d.merge(d0);
            }
        }
        return new_val(d);
    }
    virtual const char *name() const { return "merge"; }
};

class has_fields_term_t : public obj_or_seq_op_term_t {
public:
    has_fields_term_t(compile_env_t *env, const raw_term_t &term)
        : obj_or_seq_op_term_t(env, term, FILTER, argspec_t(1, -1)) { }
private:
    virtual scoped_ptr_t<val_t> obj_eval(
        eval_error *err_out, scope_env_t *env, args_t *args, const scoped_ptr_t<val_t> &v0) const {
        datum_t obj = v0->as_datum();
        r_sanity_check(obj.get_type() == datum_t::R_OBJECT);
        std::vector<datum_t> paths;
        const size_t n = args->num_args();
        paths.reserve(n - 1);
        for (size_t i = 1; i < n; ++i) {
            auto v_i = args->arg(err_out, env, i);
            if (err_out->has()) { return noval(); }

            paths.push_back(v_i->as_datum());
        }
        pathspec_t pathspec(datum_t(std::move(paths), env->env->limits()), this);
        return new_val_bool(contains(obj, pathspec));
    }
    virtual const char *name() const { return "has_fields"; }
};

class get_field_term_t : public obj_or_seq_op_term_t {
public:
    get_field_term_t(compile_env_t *env, const raw_term_t &term)
        : obj_or_seq_op_term_t(env, term, SKIP_MAP, argspec_t(2)) { }

    bool is_simple_selector() const final {
        return recursive_is_simple_selector();
    }

private:
    virtual scoped_ptr_t<val_t> obj_eval(
        eval_error *err_out, scope_env_t *env, args_t *args, const scoped_ptr_t<val_t> &v0) const {
        datum_t d = v0->as_datum();
        auto v1 = args->arg(err_out, env, 1);
        if (err_out->has()) { return noval(); }
        datum_t d_field = d.get_field_with_err(err_out, v1->as_str());
        if (err_out->has()) { return noval(); }
        return new_val(std::move(d_field));
    }
    virtual const char *name() const { return "get_field"; }
};

class bracket_term_t : public grouped_seq_op_term_t {
public:
    bracket_term_t(compile_env_t *env, const raw_term_t &term)
        : grouped_seq_op_term_t(env, term, argspec_t(2),
                                optargspec_t({"_NO_RECURSE_"})),
          impl(this, SKIP_MAP, std::set<std::string>()) {}

    bool is_simple_selector() const final {
        return recursive_is_simple_selector();
    }

private:
    scoped_ptr_t<val_t> obj_eval_dereferenced(
        eval_error *err_out, const scoped_ptr_t<val_t> &v0, const scoped_ptr_t<val_t> &v1) const {
        datum_t d = v0->as_datum();
        datum_t d_field = d.get_field_with_err(err_out, v1->as_str());
        if (err_out->has()) { return noval(); }
        return new_val(std::move(d_field));
    }
    virtual scoped_ptr_t<val_t> eval_impl(
        eval_error *err_out, scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> v0 = args->arg(err_out, env, 0);
        if (err_out->has()) { return noval(); }

        scoped_ptr_t<val_t> v1 = args->arg(err_out, env, 1);
        if (err_out->has()) { return noval(); }

        datum_t d = v1->as_datum();
        r_sanity_check(d.has());

        switch (d.get_type()) {
        case datum_t::R_NUM: {
            return nth_term_impl(this, env, std::move(v0), v1);
        }
        case datum_t::R_STR:
            return impl.eval_impl_dereferenced(
                err_out, this, env, args, v0,
                [&](eval_error *err_out) { return this->obj_eval_dereferenced(err_out, v0, v1); });
        case datum_t::MINVAL:
        case datum_t::R_ARRAY:
        case datum_t::R_BINARY:
        case datum_t::R_BOOL:
        case datum_t::R_NULL:
        case datum_t::R_OBJECT:
        case datum_t::MAXVAL:
        case datum_t::UNINITIALIZED:
        default:
            d.type_error(
                strprintf(
                    "Expected NUMBER or STRING as second argument to `%s` but found %s.",
                    name(), d.get_type_name().c_str()));
            unreachable();
        }
    }
    virtual const char *name() const { return "bracket"; }
    // obj_or_seq_op_term_t already does this, but because nth_term wasn't grouped,
    // I reimplement it here for clarity.
    virtual bool is_grouped_seq_op() const { return true; }

    obj_or_seq_op_impl_t impl;
};

counted_t<term_t> make_get_field_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<get_field_term_t>(env, term);
}

counted_t<term_t> make_bracket_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<bracket_term_t>(env, term);
}

counted_t<term_t> make_has_fields_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<has_fields_term_t>(env, term);
}

counted_t<term_t> make_pluck_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<pluck_term_t>(env, term);
}

counted_t<term_t> make_without_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<without_term_t>(env, term);
}

counted_t<term_t> make_literal_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<literal_term_t>(env, term);
}

counted_t<term_t> make_merge_term(
        compile_env_t *env, const raw_term_t &term) {
    return make_counted<merge_term_t>(env, term);
}

} // namespace ql
