#include <algorithm>

#include "rdb_protocol/op.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/terms/terms.hpp"

namespace ql {

class sample_term_t : public op_term_t {
public:
    sample_term_t(env_t *env, protob_t<const Term> term)
        : op_term_t(env, term, argspec_t(2)) { }
    counted_t<val_t> eval_impl() {
        int64_t num_int = arg(1)->as_int();
        rcheck(num_int >= 0,
               base_exc_t::GENERIC,
               strprintf("Number of items to sample must be non-negative, got `%"
                         PRId64 "`.", num_int));
        size_t num = num_int;
        counted_t<table_t> t;
        counted_t<datum_stream_t> seq;
        counted_t<val_t> v = arg(0);

        if (v->get_type().is_convertible(val_t::type_t::SELECTION)) {
            std::pair<counted_t<table_t>, counted_t<datum_stream_t> > t_seq
                = v->as_selection();
            t = t_seq.first;
            seq = t_seq.second;
        } else {
            seq = v->as_seq();
        }

        std::vector<counted_t<const datum_t> > result;
        size_t element_number = 0;
        while (counted_t<const datum_t> row = seq->next()) {
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
        }

        std::random_shuffle(result.begin(), result.end());

        counted_t<datum_stream_t> new_ds(
                new array_datum_stream_t(env, make_counted<const datum_t>(result),
                                         backtrace()));

        return t.has() ? new_val(new_ds, t) : new_val(new_ds);
    }
    bool is_deterministic_impl() const {
        return false;
    }
    virtual const char *name() const { return "sample"; }
};

counted_t<term_t> make_sample_term(env_t *env, protob_t<const Term> term) {
    return counted_t<sample_term_t>(new sample_term_t(env, term));
}

} //namespace ql
