// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/terms/terms.hpp"

#include <string>
#include <utility>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {

// NOTE: `asc` and `desc` don't fit into our type system (they're a hack for
// orderby to avoid string parsing), so we instead literally examine the
// protobuf to determine whether they're present.  This is a hack.  (This is
// implemented internally in terms of string concatenation because that's what
// the old string-parsing solution was, and this was a quick way to get the new
// behavior that also avoided dumping already-tested code paths.  I'm probably
// going to hell for it though.)

class asc_term_t : public op_term_t {
public:
    asc_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        return args->arg(env, 0);
    }
    virtual const char *name() const { return "asc"; }
};

class desc_term_t : public op_term_t {
public:
    desc_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        return args->arg(env, 0);
    }
    virtual const char *name() const { return "desc"; }
};

class orderby_term_t : public op_term_t {
public:
    orderby_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1, -1),
          optargspec_t({"index"})), src_term(term) { }
private:
    enum order_direction_t { ASC, DESC };
    class lt_cmp_t {
    public:
        typedef bool result_type;
        explicit lt_cmp_t(
            std::vector<std::pair<order_direction_t, counted_t<const func_t> > > _comparisons)
            : comparisons(std::move(_comparisons)) { }

        bool operator()(env_t *env,
                        profile::sampler_t *sampler,
                        datum_t l,
                        datum_t r) const {
            sampler->new_sample();
            for (auto it = comparisons.begin(); it != comparisons.end(); ++it) {
                datum_t lval;
                datum_t rval;
                try {
                    lval = it->second->call(env, l)->as_datum();
                } catch (const base_exc_t &e) {
                    if (e.get_type() != base_exc_t::NON_EXISTENCE) {
                        throw;
                    }
                }

                try {
                    rval = it->second->call(env, r)->as_datum();
                } catch (const base_exc_t &e) {
                    if (e.get_type() != base_exc_t::NON_EXISTENCE) {
                        throw;
                    }
                }

                if (!lval.has() && !rval.has()) {
                    continue;
                }
                if (!lval.has()) {
                    return true != (it->first == DESC);
                }
                if (!rval.has()) {
                    return false != (it->first == DESC);
                }
                int cmp_res = lval.cmp(env->reql_version(), rval);
                if (cmp_res == 0) {
                    continue;
                }
                return (cmp_res < 0) != (it->first == DESC);
            }

            return false;
        }

    private:
        const std::vector<std::pair<order_direction_t, counted_t<const func_t> > >
            comparisons;
    };

    virtual scoped_ptr_t<val_t>
    eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        std::vector<std::pair<order_direction_t, counted_t<const func_t> > > comparisons;
        for (int i = 0; i < get_src()->args_size(); ++i) {
            // Abort if any of the arguments is `r.args`. We don't currently
            // support that with `order_by`.
            rcheck(get_src()->args(i).type() != Term::ARGS, base_exc_t::GENERIC,
                   "r.args is not supported in an order_by command yet.");
        }
        r_sanity_check(static_cast<size_t>(get_src()->args_size()) == args->num_args());
        for (size_t i = 1; i < args->num_args(); ++i) {
            if (get_src()->args(i).type() == Term::DESC) {
                comparisons.push_back(
                    std::make_pair(
                        DESC, args->arg(env, i)->as_func(GET_FIELD_SHORTCUT)));
            } else {
                comparisons.push_back(
                    std::make_pair(
                        ASC, args->arg(env, i)->as_func(GET_FIELD_SHORTCUT)));
            }
        }
        lt_cmp_t lt_cmp(comparisons);

        counted_t<table_slice_t> tbl_slice;
        counted_t<datum_stream_t> seq;
        scoped_ptr_t<val_t> v0 = args->arg(env, 0);
        if (v0->get_type().is_convertible(val_t::type_t::TABLE_SLICE)) {
            tbl_slice = v0->as_table_slice();
        } else if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            auto selection = v0->as_selection(env->env);
            tbl_slice = make_counted<table_slice_t>(selection->table);
            seq = selection->seq;
        } else {
            seq = v0->as_seq(env->env);
        }

        scoped_ptr_t<val_t> index = args->optarg(env, "index");
        if (seq.has() && seq->is_exhausted()){
            /* Do nothing for empty sequence */
            if (!index.has()) {
                rcheck(!comparisons.empty(), base_exc_t::GENERIC,
                       "Must specify something to order by.");
            }
        /* Add a sorting to the table if we're doing indexed sorting. */
        } else if (index.has()) {
            rcheck(tbl_slice.has(), base_exc_t::GENERIC,
                   "Indexed order_by can only be performed on a TABLE or TABLE_SLICE.");
            rcheck(!seq.has(), base_exc_t::GENERIC,
                   "Indexed order_by can only be performed on a TABLE or TABLE_SLICE.");
            sorting_t sorting = sorting_t::UNORDERED;
            for (int i = 0; i < get_src()->optargs_size(); ++i) {
                if (get_src()->optargs(i).key() == "index") {
                    if (get_src()->optargs(i).val().type() == Term::DESC) {
                        sorting = sorting_t::DESCENDING;
                    } else {
                        sorting = sorting_t::ASCENDING;
                    }
                }
            }
            r_sanity_check(sorting != sorting_t::UNORDERED);
            std::string index_str = index->as_str().to_std();
            tbl_slice = tbl_slice->with_sorting(index_str, sorting);
            if (!comparisons.empty()) {
                seq = make_counted<indexed_sort_datum_stream_t>(
                    tbl_slice->as_seq(env->env, backtrace()), lt_cmp);
            } else {
                return new_val(tbl_slice);
            }
        } else {
            if (!seq.has()) {
                seq = tbl_slice->as_seq(env->env, backtrace());
            }
            rcheck(!comparisons.empty(), base_exc_t::GENERIC,
                   "Must specify something to order by.");
            std::vector<datum_t> to_sort;
            batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
            for (;;) {
                std::vector<datum_t> data
                    = seq->next_batch(env->env, batchspec);
                if (data.size() == 0) {
                    break;
                }
                std::move(data.begin(), data.end(), std::back_inserter(to_sort));
                rcheck_array_size(to_sort, env->env->limits(), base_exc_t::GENERIC);
            }
            profile::sampler_t sampler("Sorting in-memory.", env->env->trace);
            auto fn = boost::bind(lt_cmp, env->env, &sampler, _1, _2);
            std::stable_sort(to_sort.begin(), to_sort.end(), fn);
            seq = make_counted<array_datum_stream_t>(
                datum_t(std::move(to_sort), env->env->limits()),
                backtrace());
        }
        return tbl_slice.has()
            ? new_val(make_counted<selection_t>(tbl_slice->get_tbl(), seq))
            : new_val(env->env, seq);
    }

    virtual const char *name() const { return "orderby"; }

private:
    protob_t<const Term> src_term;
};

class distinct_term_t : public op_term_t {
public:
    distinct_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1), optargspec_t({"index"})) { }
private:
    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args,
                                       eval_flags_t) const {
        scoped_ptr_t<val_t> v = args->arg(env, 0);
        scoped_ptr_t<val_t> idx = args->optarg(env, "index");
        if (v->get_type().is_convertible(val_t::type_t::TABLE_SLICE)) {
            counted_t<table_slice_t> tbl_slice = v->as_table_slice();
            std::string tbl_pkey = tbl_slice->get_tbl()->get_pkey();
            std::string idx_str = idx.has() ? idx->as_str().to_std() : tbl_pkey;
            if (idx.has() && idx_str == tbl_pkey) {
                auto row = pb::dummy_var_t::DISTINCT_ROW;
                std::vector<sym_t> distinct_args{dummy_var_to_sym(row)}; // NOLINT(readability/braces) yes we bloody well do need the ;
                protob_t<Term> body(make_counted_term());
                {
                    r::reql_t f = r::var(row)[idx_str];
                    body->Swap(&f.get());
                }
                propagate(body.get());
                counted_t<datum_stream_t> s = tbl_slice->as_seq(env->env, backtrace());
                map_wire_func_t mwf(body, std::move(distinct_args), backtrace());
                s->add_transformation(std::move(mwf), backtrace());
                return new_val(env->env, s);
            } else if (!tbl_slice->get_idx() || *tbl_slice->get_idx() == idx_str) {
                if (tbl_slice->sorting == sorting_t::UNORDERED) {
                    tbl_slice = tbl_slice->with_sorting(idx_str, sorting_t::ASCENDING);
                }
                counted_t<datum_stream_t> s = tbl_slice->as_seq(env->env, backtrace());
                s->add_transformation(distinct_wire_func_t(idx.has()), backtrace());
                return new_val(env->env, s->ordered_distinct());
            }
        }
        rcheck(!idx, base_exc_t::GENERIC,
               "Can only perform an indexed distinct on a TABLE.");
        counted_t<datum_stream_t> s = v->as_seq(env->env);
        // The reql_version matters here, because we copy `results` into `toret`
        // in ascending order.
        std::set<datum_t, optional_datum_less_t>
            results(optional_datum_less_t(env->env->reql_version()));
        batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
        {
            profile::sampler_t sampler("Evaluating elements in distinct.",
                                       env->env->trace);
            datum_t d;
            while (d = s->next(env->env, batchspec), d.has()) {
                results.insert(std::move(d));
                rcheck_array_size(results, env->env->limits(), base_exc_t::GENERIC);
                sampler.new_sample();
            }
        }
        std::vector<datum_t> toret;
        std::move(results.begin(), results.end(), std::back_inserter(toret));
        return new_val(datum_t(std::move(toret), env->env->limits()));
    }

    virtual const char *name() const { return "distinct"; }
};

counted_t<term_t> make_orderby_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<orderby_term_t>(env, term);
}
counted_t<term_t> make_distinct_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<distinct_term_t>(env, term);
}
counted_t<term_t> make_asc_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<asc_term_t>(env, term);
}
counted_t<term_t> make_desc_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<desc_term_t>(env, term);
}

} // namespace ql
