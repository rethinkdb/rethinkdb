#include "rdb_protocol/terms/terms.hpp"

#include <string>
#include <utility>
#include <vector>

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pb_utils.hpp"

namespace ql {

// Most of the real logic for these is in datum_stream.cc.

class count_term_t : public op_term_t {
public:
    count_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, 2)) { }
private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        if (num_args() == 1) {
            return new_val(arg(env, 0)->as_seq(env)->count(env));
        } else if (arg(env, 1)->get_type().is_convertible(val_t::type_t::FUNC)) {
            return new_val(arg(env, 0)->as_seq(env)->filter(env, arg(env, 1)->as_func(env), counted_t<func_t>())->count(env));
        } else {
            counted_t<func_t> f =
                func_t::new_eq_comparison_func(env, arg(env, 1)->as_datum(), backtrace());
            return new_val(arg(env, 0)->as_seq(env)->filter(env, f, counted_t<func_t>())->count(env));
        }
    }
    virtual const char *name() const { return "count"; }
};

class map_term_t : public op_term_t {
public:
    map_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        return new_val(env, arg(env, 0)->as_seq(env)->map(env, arg(env, 1)->as_func(env)));
    }
    virtual const char *name() const { return "map"; }
};

class concatmap_term_t : public op_term_t {
public:
    concatmap_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }
private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        return new_val(env, arg(env, 0)->as_seq(env)->concatmap(env, arg(env, 1)->as_func(env)));
    }
    virtual const char *name() const { return "concatmap"; }
};

class filter_term_t : public op_term_t {
public:
    filter_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2), optargspec_t({"default"})),
          default_filter_val(lazy_literal_optarg(env, "default")) { }
private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        counted_t<val_t> v0 = arg(env, 0);
        counted_t<val_t> v1 = arg(env, 1, LITERAL_OK);
        counted_t<func_t> f = v1->as_func(env, CONSTANT_SHORTCUT);
        if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            std::pair<counted_t<table_t>, counted_t<datum_stream_t> > ts
                = v0->as_selection(env);
            return new_val(ts.second->filter(env, f, default_filter_val), ts.first);
        } else {
            return new_val(env, v0->as_seq(env)->filter(env, f, default_filter_val));
        }
    }
    counted_t<func_t> default_filter_val;
    virtual const char *name() const { return "filter"; }
};

class reduce_term_t : public op_term_t {
public:
    reduce_term_t(env_t *env, const protob_t<const Term> &term) :
        op_term_t(env, term, argspec_t(2), optargspec_t({ "base" })) { }
private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        return new_val(arg(env, 0)->as_seq(env)->reduce(env,
                                                   optarg(env, "base"),
                                                   arg(env, 1)->as_func(env)));
    }
    virtual const char *name() const { return "reduce"; }
};

// TODO: this sucks.  Change to use the same macros as rewrites.hpp?
class between_term_t : public bounded_op_term_t {
public:
    between_term_t(env_t *env, const protob_t<const Term> &term)
        : bounded_op_term_t(env, term, argspec_t(3), optargspec_t({"index"})) { }
private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        counted_t<table_t> tbl = arg(env, 0)->as_table();
        counted_t<const datum_t> lb = arg(env, 1)->as_datum();
        if (lb->get_type() == datum_t::R_NULL) {
            lb.reset();
        }
        counted_t<const datum_t> rb = arg(env, 2)->as_datum();
        if (rb->get_type() == datum_t::R_NULL) {
            rb.reset();
        }

        if (lb.has() && rb.has()) {
            if (*lb > *rb || ((left_open(env) || right_open(env)) && *lb == *rb)) {
                counted_t<const datum_t> arr = make_counted<datum_t>(datum_t::R_ARRAY);
                counted_t<datum_stream_t> ds(
                    new array_datum_stream_t(arr, backtrace()));
                return new_val(ds, tbl);
            }
        }

        counted_t<val_t> sindex = optarg(env, "index");
        std::string sid = (sindex.has() ? sindex->as_str() : tbl->get_pkey());

        tbl->add_bounds(lb, left_open(env), rb, right_open(env), sid, this);
        return new_val(tbl);
    }
    virtual const char *name() const { return "between"; }

    protob_t<Term> filter_func;
};

class union_term_t : public op_term_t {
public:
    union_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(0, -1)) { }
private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        std::vector<counted_t<datum_stream_t> > streams;
        for (size_t i = 0; i < num_args(); ++i) {
            streams.push_back(arg(env, i)->as_seq(env));
        }
        counted_t<datum_stream_t> union_stream
            = make_counted<union_datum_stream_t>(streams, backtrace());
        return new_val(env, union_stream);
    }
    virtual const char *name() const { return "union"; }
};

class zip_term_t : public op_term_t {
public:
    zip_term_t(env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(env_t *env, UNUSED eval_flags_t flags) {
        return new_val(env, arg(env, 0)->as_seq(env)->zip());
    }
    virtual const char *name() const { return "zip"; }
};

counted_t<term_t> make_between_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<between_term_t>(env, term);
}
counted_t<term_t> make_reduce_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<reduce_term_t>(env, term);
}
counted_t<term_t> make_map_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<map_term_t>(env, term);
}
counted_t<term_t> make_filter_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<filter_term_t>(env, term);
}
counted_t<term_t> make_concatmap_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<concatmap_term_t>(env, term);
}
counted_t<term_t> make_count_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<count_term_t>(env, term);
}
counted_t<term_t> make_union_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<union_term_t>(env, term);
}
counted_t<term_t> make_zip_term(env_t *env, const protob_t<const Term> &term) {
    return make_counted<zip_term_t>(env, term);
}

} // namespace ql
