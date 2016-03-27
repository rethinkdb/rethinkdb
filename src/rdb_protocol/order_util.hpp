
// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ORDER_UTIL_HPP_
#define RDB_PROTOCOL_ORDER_UTIL_HPP_

#include <string>
#include <utility>

#include "errors.hpp"

#include "rdb_protocol/error.hpp"
#include "rdb_protocol/profile.hpp"
#include "containers/counted.hpp"

namespace ql {

enum order_direction_t { ASC, DESC };

class scope_env_t;
class env_t;
class var_scope_t;
class func_t;
class raw_term_t;
class term_t;
class args_t;

void check_r_args(const term_t *target, const raw_term_t &term);

std::vector<std::pair<order_direction_t, counted_t<const func_t> > >
build_comparisons_from_raw_term(const term_t *target,
                                scope_env_t *env,
                                args_t *args,
                                const raw_term_t &raw_term);

std::vector<std::pair<order_direction_t, counted_t<const func_t> > >
build_comparisons_from_optional_terms(const term_t *target,
                                     scope_env_t *env,
                                     std::vector<scoped_ptr_t<val_t> > &&args,
                                     const raw_term_t &raw_term);
std::vector<std::pair<order_direction_t, counted_t<const func_t> > >
build_comparisons_from_single_term(const term_t *target,
                                   scope_env_t *,
                                   scoped_ptr_t<val_t> &&arg,
                                   const raw_term_t &raw_term);

class lt_cmp_t {
public:
    typedef bool result_type;
    explicit lt_cmp_t(
        std::vector<std::pair<order_direction_t, counted_t<const func_t> > > _comparisons);
    // You can call this with nullptr as sampler to not sample.
    bool operator()(env_t *env,
                    profile::sampler_t *sampler,
                    datum_t l,
                    datum_t r) const;

private:
    const std::vector<std::pair<order_direction_t, counted_t<const func_t> > >
        comparisons;
};

} // namespace ql

#endif
// RDB_PROTOCOL_ORDER_UTIL_HPP_
