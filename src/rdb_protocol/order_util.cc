// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/order_util.hpp"

#include <string>
#include <utility>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term_storage.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {

void check_r_args(const term_t *target, const raw_term_t &term) {
    // Abort if any of the arguments is `r.args`. We don't currently
    // support that with `order_by` or `union`.
    rcheck_target(target, term.type() != Term::ARGS, base_exc_t::LOGIC,
           "r.args is not supported in an order_by or union command yet.");
}

std::pair<order_direction_t, counted_t<const func_t> >
build_comparison(const term_t* target,
                      scoped_ptr_t<val_t> arg,
                      raw_term_t item) {

    check_r_args(target, item);

    if (item.type() == Term::DESC) {
        return std::make_pair(DESC,
                              arg->as_func(GET_FIELD_SHORTCUT));
    } else {
        return std::make_pair(ASC,
                              arg->as_func(GET_FIELD_SHORTCUT));
    }
}

std::vector<std::pair<order_direction_t, counted_t<const func_t> > >
build_comparisons_from_raw_term(const term_t *target,
                                scope_env_t *env,
                                args_t *args,
                                const raw_term_t &raw_term) {
    std::vector<std::pair<order_direction_t, counted_t<const func_t> > > comparisons;

    check_r_args(target, raw_term.arg(0));
    // For order by, first argument is the stream
    for (size_t i = 1; i < raw_term.num_args(); ++i) {
        raw_term_t item = raw_term.arg(i);
        comparisons.push_back(build_comparison(target,
                                               args->arg(env, i),
                                               item));
    }
    return comparisons;
}

std::vector<std::pair<order_direction_t, counted_t<const func_t> > >
build_comparisons_from_single_term(const term_t *target,
                                   scope_env_t *,
                                   scoped_ptr_t<val_t> &&arg,
                                   const raw_term_t &raw_term) {
    std::vector<std::pair<order_direction_t, counted_t<const func_t> > > comparisons;

    comparisons.push_back(build_comparison(target,
                                           std::move(arg),
                                           raw_term));

    return std::move(comparisons);
}
std::vector<std::pair<order_direction_t, counted_t<const func_t> > >
build_comparisons_from_optional_terms(const term_t *target,
                                     scope_env_t *,
                                     std::vector<scoped_ptr_t<val_t> > &&args,
                                     const raw_term_t &raw_term)
{
    std::vector<std::pair<order_direction_t, counted_t<const func_t> > > comparisons;

    // For ordered union, first element is an optional arg
    for (size_t i = 0; i < raw_term.num_args(); ++i) {
        raw_term_t item = raw_term.arg(i);
        comparisons.push_back(build_comparison(target,
                                               std::move(args[i]),
                                               item));
    }
    return std::move(comparisons);
}

lt_cmp_t::lt_cmp_t(std::vector<std::pair<order_direction_t, counted_t<const func_t> > > _comparisons)
            : comparisons(std::move(_comparisons)) { }

bool lt_cmp_t::operator()(env_t *env,
                          profile::sampler_t *sampler,
                          datum_t l,
                          datum_t r) const {

    if (sampler != nullptr) {
        sampler->new_sample();
    }

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
        int cmp_res = lval.cmp(rval);
        if (cmp_res == 0) {
            continue;
        }
        return (cmp_res < 0) != (it->first == DESC);
    }

    return false;
}

} // namespace ql
