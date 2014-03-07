
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

struct world_t;
BOOST_TYPEOF_REGISTER_TYPE(world_t)

//[world_void
struct world_t {
    std::vector<person> persons;
    bool commit;
} world; // Global variable.

void add_person(person const& a_person) {
    world.commit = false;
    world.persons.push_back(a_person);

    BOOST_SCOPE_EXIT(void) { // No captures.
        if(!world.commit) world.persons.pop_back();
    } BOOST_SCOPE_EXIT_END

    // ...

    world.commit = true;
}
//]

int main(void) {
    person p;
    add_person(p);
    BOOST_TEST(world.persons.size() == 1);
    return boost::report_errors();
}

