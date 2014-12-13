// Copyright 2010-2013 RethinkDB, all rights reserved.
#include <algorithm>
#include <limits>
#include <cmath>

#include "math.hpp"
#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/math_utils.hpp"
#include "rdb_protocol/terms/terms.hpp"

namespace ql {

class sample_term_t : public op_term_t {
public:
    sample_term_t(compile_env_t *env, const protob_t<const Term> &term)
        : op_term_t(env, term, argspec_t(2)) { }

    scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        int64_t num_int = args->arg(env, 1)->as_int();
        rcheck(num_int >= 0,
               base_exc_t::GENERIC,
               strprintf("Number of items to sample must be non-negative, got `%"
                         PRId64 "`.", num_int));
        const size_t num = num_int;
        counted_t<table_t> t;
        counted_t<datum_stream_t> seq;
        scoped_ptr_t<val_t> v = args->arg(env, 0);

        if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
            counted_t<selection_t> t_seq = v->as_selection(env->env);
            t = t_seq->table;
            seq = t_seq->seq;
        } else {
            seq = v->as_seq(env->env);
        }

        std::vector<datum_t> result;
        result.reserve(num);
        size_t element_number = 0;
        batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env->env);
        {
            profile::sampler_t sampler("Sampling elements.", env->env->trace);
            datum_t row;
            while (row = seq->next(env->env, batchspec), row.has()) {
                element_number++;
                if (result.size() < num) {
                    result.push_back(row);
                } else {
                    /* We have a limitation on the size of arrays that makes
                     * sure they are less than the size of an integer. */
                    size_t new_index = randint(element_number);
                    if (new_index < num) {
                        result[new_index] = row;
                    }
                }
                sampler.new_sample();
            }
        }

        std::random_shuffle(result.begin(), result.end());

        counted_t<datum_stream_t> new_ds(
            new array_datum_stream_t(datum_t(std::move(result), env->env->limits()),
                                     backtrace()));

        return t.has()
            ? new_val(make_counted<selection_t>(t, new_ds))
            : new_val(env->env, new_ds);
    }

    bool is_deterministic() const {
        return false;
    }

    virtual const char *name() const { return "sample"; }
};

class random_term_t : public op_term_t {
public:
    random_term_t(compile_env_t *env, const protob_t<const Term> &term) :
        op_term_t(env, term, argspec_t(0, 2), optargspec_t({"float"})) {
    }
private:
    virtual bool is_deterministic() const {
        return false;
    }

    enum class bound_type_t {
        LOWER,
        UPPER
    };

    int64_t convert_bound(double bound, bound_type_t type) const {
        int64_t res;
        bool success = number_as_integer(bound, &res);
        rcheck(success, base_exc_t::GENERIC,
               strprintf("%s bound (%" PR_RECONSTRUCTABLE_DOUBLE ") could not be safely converted to an integer.",
                         type == bound_type_t::LOWER ? "Lower" : "Upper", bound));
        return res;
    }

    virtual scoped_ptr_t<val_t> eval_impl(scope_env_t *env, args_t *args, eval_flags_t) const {
        scoped_ptr_t<val_t> use_float_arg = args->optarg(env, "float");
        bool use_float = use_float_arg ? use_float_arg->as_bool() : args->num_args() == 0;

        if (use_float) {
            double lower = 0.0;
            double upper = 1.0;

            if (args->num_args() == 0) {
                // Use default bounds
            } else if (args->num_args() == 1) {
                upper = args->arg(env, 0)->as_num();
            } else {
                r_sanity_check(args->num_args() == 2);
                lower = args->arg(env, 0)->as_num();
                upper = args->arg(env, 1)->as_num();
            }

            bool range_scaled = false;
            double result;

            // Range may overflow, so check for that case
            if ((lower > 0.0) != (upper > 0.0) &&
                std::abs(lower) > (std::numeric_limits<double>::max() / 2) - std::abs(upper))
            {
                // Do the random generation on half the range, then scale it later
                range_scaled = true;
                lower = lower / 4.0;
                upper = upper / 4.0;
            }

            result = lower + (randdouble() * (upper - lower));

            // The roll above can result in the upper value.  If this happens, we
            // return the lower value, which keeps things fair.
            if (upper > lower ? result >= upper : result <= upper) {
                result = lower;
            }

            if (range_scaled) {
                result = result * 4.0;
            }

            return new_val(datum_t(result));
        } else {
            rcheck(args->num_args() > 0, base_exc_t::GENERIC,
                   "Generating a random integer requires one or two bounds.");
            int64_t lower;
            int64_t upper;

            // Load the lower and upper values, and reject the query if we could
            // lose precision when putting the result in a datum_t (double)
            if (args->num_args() == 1) {
                lower = 0;
                upper = convert_bound(args->arg(env, 0)->as_num(), bound_type_t::UPPER);
            } else {
                r_sanity_check(args->num_args() == 2);
                lower = convert_bound(args->arg(env, 0)->as_num(), bound_type_t::LOWER);
                upper = convert_bound(args->arg(env, 1)->as_num(), bound_type_t::UPPER);
            }

            rcheck(lower < upper, base_exc_t::GENERIC,
                   strprintf("Lower bound (%" PRIi64 ") is not less than upper bound (%" PRIi64 ").",
                             lower, upper));

            // This stuff is to ensure a uniform distribution
            // Round range up to the nearest power of two
            uint64_t range = upper - lower;
            uint64_t max_rand = uint64_round_up_to_power_of_two(range);
            uint64_t result;
            do {
                result = randuint64(max_rand);
            } while (result >= range);


            int64_t signed_result = lower;
            signed_result += result;

            return new_val(datum_t(safe_to_double(signed_result)));
        }
    }

    virtual const char *name() const { return "random"; }
};

counted_t<term_t> make_sample_term(compile_env_t *env, const protob_t<const Term> &term) {
    return counted_t<sample_term_t>(new sample_term_t(env, term));
}
counted_t<term_t> make_random_term(compile_env_t *env, const protob_t<const Term> &term) {
    return make_counted<random_term_t>(env, term);
}

} // namespace ql
