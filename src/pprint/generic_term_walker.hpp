// Copyright 2015 RethinkDB, all rights reserved
#ifndef PPRINT_GENERIC_TERM_WALKER_HPP_
#define PPRINT_GENERIC_TERM_WALKER_HPP_

namespace ql {
class raw_term_t;
}

namespace pprint {

template <typename Accumulator>
class generic_term_walker_t {
public:
    virtual ~generic_term_walker_t() {}

    virtual Accumulator walk(const ql::raw_term_t &t) {
        return visit_generic(t);
    }
protected:
    virtual Accumulator visit_generic(const ql::raw_term_t &t) = 0;
};

} // namespace pprint

#endif // PPRINT_GENERIC_TERM_WALKER_HPP_
