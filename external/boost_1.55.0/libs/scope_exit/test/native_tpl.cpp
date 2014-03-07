
// Copyright (C) 2006-2009, 2012 Alexander Nasonov
// Copyright (C) 2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/scope_exit

#include <boost/scope_exit.hpp>
#include <boost/rational.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/vector.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <vector>

template<class type>
void tpl_long(
      type tval
    , type & t
    , type const& tc
    , type volatile& tv
    , type const volatile& tcv
) {
    int i = 0; // non-dependent name
    type const remember(tval);

    {
        BOOST_SCOPE_EXIT_TPL( (&tval) (&t) (&tc) (&tv) (&tcv) (&i) ) {
            tval = 1;
            ++t;
            ++tv;
        } BOOST_SCOPE_EXIT_END

        BOOST_TEST(t == remember);
        BOOST_TEST(tval == remember);
    }

    BOOST_TEST(tval == 1);
    BOOST_TEST(t == remember + 2);
}

template<class Vector, int Value>
void tpl_vector(
      Vector vval
    , Vector & v
    , Vector const& vc
) {
    Vector const remember(vval);

    {
        BOOST_SCOPE_EXIT_TPL( (&vval) (&v) (&vc) ) {
            v.push_back(-Value);
            vval.push_back(Value);
        } BOOST_SCOPE_EXIT_END

        BOOST_TEST(v.size() == remember.size());
        BOOST_TEST(vval.size() == remember.size());
    }

    BOOST_TEST(v.size() == 1 + remember.size());
    BOOST_TEST(vval.size() == 1 + remember.size());
}

int main(void) {
    long l = 137;
    tpl_long(l, l, l, l, l);

    std::vector<int> v(10, 137);
    tpl_vector<std::vector<int>, 13>(v, v, v);

    return boost::report_errors();
}

