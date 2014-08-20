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
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        return args->arg(env, 0);
    }
    virtual const char *name() const { return "asc"; }
};

class desc_term_t : public op_term_t {
public:
    desc_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(1)) { }
private:
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
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
                        counted_t<const datum_t> l,
                        counted_t<const datum_t> r) const {
            sampler->new_sample();
            for (auto it = comparisons.begin(); it != comparisons.end(); ++it) {
                counted_t<const datum_t> lval;
                counted_t<const datum_t> rval;
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
                // TODO(2014-08): use datum_t::cmp instead to be faster
                if (*lval == *rval) {
                    continue;
                }
                return lval->compare_lt(env->reql_version(), *rval) !=
                    (it->first == DESC);
            }

            return false;
        }

    private:
        const std::vector<std::pair<order_direction_t, counted_t<const func_t> > >
            comparisons;
    };

    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        std::vector<std::pair<order_direction_t, counted_t<const func_t> > > comparisons;
        for (size_t i = 1; i < args->num_args(); ++i) {
            if (get_src()->args(i).type() == Term::DESC) {
                comparisons.push_back(
                        std::make_pair(DESC, args->arg(env, i)->as_func(GET_FIELD_SHORTCUT)));
            } else {
                comparisons.push_back(
                        std::make_pair(ASC, args->arg(env, i)->as_func(GET_FIELD_SHORTCUT)));
            }
        }
        lt_cmp_t lt_cmp(comparisons);

        counted_t<table_t> tbl;
        counted_t<datum_stream_t> seq;
        counted_t<val_t> v0 = args->arg(env, 0);
        if (v0->get_type().is_convertible(val_t::type_t::TABLE)) {
            tbl = v0->as_table();
        } else if (v0->get_type().is_convertible(val_t::type_t::SELECTION)) {
            std::pair<counted_t<table_t>, counted_t<datum_stream_t> > ts
                = v0->as_selection(env->env);
            tbl = ts.first;
            seq = ts.second;
        } else {
            seq = v0->as_seq(env->env);
        }

        counted_t<val_t> index = args->optarg(env, "index");
        if (seq.has() && seq->is_exhausted()){
            /* Do nothing for empty sequence */
            if (!index.has()) {
                rcheck(!comparisons.empty(), base_exc_t::GENERIC,
                       "Must specify something to order by.");
            }
        /* Add a sorting to the table if we're doing indexed sorting. */
        } else if (index.has()) {
            rcheck(tbl.has(), base_exc_t::GENERIC,
                   "Indexed order_by can only be performed on a TABLE.");
            rcheck(!seq.has(), base_exc_t::GENERIC,
                   "Indexed order_by can only be performed on a TABLE.");
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
            tbl->add_sorting(index_str, sorting, this);
            if (index_str != tbl->get_pkey()
                && !comparisons.empty()) {
                seq = make_counted<indexed_sort_datum_stream_t>(
                    tbl->as_datum_stream(env->env, backtrace()), lt_cmp);
            } else {
                seq = tbl->as_datum_stream(env->env, backtrace());
            }
        } else {
            if (!seq.has()) {
                seq = tbl->as_datum_stream(env->env, backtrace());
            }
            rcheck(!comparisons.empty(), base_exc_t::GENERIC,
                   "Must specify something to order by.");
            std::vector<counted_t<const datum_t> > to_sort;
            batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
            for (;;) {
                std::vector<counted_t<const datum_t> > data
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
                make_counted<const datum_t>(std::move(to_sort), env->env->limits()),
                backtrace());
        }
        return tbl.has() ? new_val(seq, tbl) : new_val(env->env, seq);
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
    virtual counted_t<val_t> eval_impl(scope_env_t *env, args_t *args,
                                       eval_flags_t) const {
        counted_t<val_t> v = args->arg(env, 0);
        counted_t<val_t> idx = args->optarg(env, "index");
        if (v->get_type().is_convertible(val_t::type_t::TABLE)) {
            counted_t<table_t> tbl = v->as_table();
            std::string idx_str = idx.has() ? idx->as_str().to_std() : tbl->get_pkey();
            if (idx.has() && idx_str == tbl->get_pkey()) {
                auto row = pb::dummy_var_t::DISTINCT_ROW;
                std::vector<sym_t> distinct_args{dummy_var_to_sym(row)}; // NOLINT(readability/braces) yes we bloody well do need the ;
                protob_t<Term> body(make_counted_term());
                {
                    r::reql_t f = r::var(row)[idx_str];
                    body->Swap(&f.get());
                }
                propagate(body.get());
                counted_t<datum_stream_t> s =
                    tbl->as_datum_stream(env->env, backtrace());
                map_wire_func_t mwf(body, std::move(distinct_args), backtrace());
                s->add_transformation(std::move(mwf), backtrace());
                return new_val(env->env, s);
            } else {
                tbl->add_sorting(idx_str, sorting_t::ASCENDING, this);
                counted_t<datum_stream_t> s =
                    tbl->as_datum_stream(env->env, backtrace());
                s->add_transformation(distinct_wire_func_t(idx.has()), backtrace());
                return new_val(env->env, s->ordered_distinct());
            }
        } else {
            rcheck(!idx, base_exc_t::GENERIC,
                   "Can only perform an indexed distinct on a TABLE.");
            counted_t<datum_stream_t> s = v->as_seq(env->env);
            // The reql_version matters here, because we copy `results` into `toret`
            // in ascending order.
            std::set<counted_t<const datum_t>, counted_datum_less_t>
                results(counted_datum_less_t(env->env->reql_version()));
            batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
            {
                profile::sampler_t sampler("Evaluating elements in distinct.",
                                           env->env->trace);
                while (counted_t<const datum_t> d = s->next(env->env, batchspec)) {
                    results.insert(std::move(d));
                    rcheck_array_size(results, env->env->limits(), base_exc_t::GENERIC);
                    sampler.new_sample();
                }
            }
            std::vector<counted_t<const datum_t> > toret;
            std::move(results.begin(), results.end(), std::back_inserter(toret));
            return new_val(make_counted<const datum_t>(std::move(toret),
                                                       env->env->limits()));
        }
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
