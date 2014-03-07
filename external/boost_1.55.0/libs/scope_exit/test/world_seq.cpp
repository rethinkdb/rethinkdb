
// Copyright (C) 2006-2009, 2012 Alexander Nasonov
// Copyright (C) 2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/scope_exit

#include <boost/scope_exit.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/vector.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <boost/detail/lightweight_test.hpp>
#include <vector>

struct person {};
BOOST_TYPEOF_REGISTER_TYPE(person)

struct world {
    void add_person(person const& a_person);
    size_t population(void) const { return persons_.size(); }

private:
    std::vector<person> persons_;
};
BOOST_TYPEOF_REGISTER_TYPE(world)

//[world_seq
void world::add_person(person const& a_person) {
    bool commit = false;

    persons_.push_back(a_person);           // (1) direct action
    // Following block is executed when the enclosing scope exits.
    BOOST_SCOPE_EXIT( (&commit) (&persons_) ) {
        if(!commit) persons_.pop_back();    // (2) rollback action
    } BOOST_SCOPE_EXIT_END

    // ...                                  // (3) other operations

    commit = true;                          // (4) disable rollback actions
}
//]

int main(void) {
    world w;
    person p;
    w.add_person(p);
    BOOST_TEST(w.population() == 1);
    return boost::report_errors();
}

