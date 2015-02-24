// Copyright 2015 RethinkDB, all rights reserved
#ifndef PPRINT_GENERIC_TERM_WALKER_HPP_
#define PPRINT_GENERIC_TERM_WALKER_HPP_

class Term;

namespace pprint {

template <typename Accumulator>
class generic_term_walker_t {
public:
    virtual ~generic_term_walker_t() {}

    virtual Accumulator walk(Term *t) {
        return visit_generic(t);
    }
protected:
    virtual Accumulator visit_generic(Term *t) = 0;
};

} // namespace pprint

#endif // PPRINT_GENERIC_TERM_WALKER_HPP_
