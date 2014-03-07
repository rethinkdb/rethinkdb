
// Copyright (C) 2006-2009, 2012 Alexander Nasonov
// Copyright (C) 2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/scope_exit

#include <boost/scope_exit.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/vector.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
#include <boost/detail/lightweight_test.hpp>
#include <vector>
#include <sstream>

struct person {
    typedef unsigned int id_t;
    typedef unsigned int evolution_t;
    
    id_t id;
    evolution_t evolution;

    person(void) : id(0), evolution(0) {}

    friend std::ostream& operator<<(std::ostream& o, person const& p) {
        return o << "person(" << p.id << ", " << p.evolution << ")";
    }
};
BOOST_TYPEOF_REGISTER_TYPE(person)

struct world {
    world(void) : next_id_(1) {}
    void add_person(person const& a_person);

    friend std::ostream& operator<<(std::ostream& o, world const& w) {
        o << "world(" << w.next_id_ << ", {";
        BOOST_FOREACH(person const& p, w.persons_) {
            o << " " << p << ", ";
        }
        return o << "})";
    }

private:
    person::id_t next_id_;
    std::vector<person> persons_;
};
BOOST_TYPEOF_REGISTER_TYPE(world)

void world::add_person(person const& a_person) {
    persons_.push_back(a_person);

    // This block must be no-throw.
    person& p = persons_.back();
    person::evolution_t checkpoint = p.evolution;
    BOOST_SCOPE_EXIT( (checkpoint) (&p) (&persons_) ) {
        if(checkpoint == p.evolution) persons_.pop_back();
    } BOOST_SCOPE_EXIT_END

    // ...

    checkpoint = ++p.evolution;

    // Assign new identifier to the person.
    person::id_t const prev_id = p.id;
    p.id = next_id_++;
    BOOST_SCOPE_EXIT( (checkpoint) (&p) (&next_id_) (prev_id) ) {
        if(checkpoint == p.evolution) {
            next_id_ = p.id;
            p.id = prev_id;
        }
    } BOOST_SCOPE_EXIT_END

    // ...

    checkpoint = ++p.evolution;
}

int main(void) {
    person adam, eva;
    std::ostringstream oss;
    oss << adam;
    BOOST_TEST(oss.str() == "person(0, 0)");

    oss.str("");
    oss << eva;
    BOOST_TEST(oss.str() == "person(0, 0)");

    world w;
    w.add_person(adam);
    w.add_person(eva);
    oss.str("");
    oss << w;
    BOOST_TEST(oss.str() == "world(3, { person(1, 2),  person(2, 2), })");

    return boost::report_errors();
}

