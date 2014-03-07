// Boost.TypeErasure library
//
// Copyright 2012 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: test_binding.cpp 78460 2012-05-13 19:58:42Z steven_watanabe $

#include <boost/type_erasure/static_binding.hpp>
#include <boost/type_erasure/binding.hpp>
#include <boost/type_erasure/placeholder.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/map.hpp>

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace boost::type_erasure;

BOOST_AUTO_TEST_CASE(test_empty_binding)
{
    boost::mpl::map<> m1;
    binding<boost::mpl::vector<> > b1(m1);
    binding<boost::mpl::vector<> > b2(make_binding<boost::mpl::map<> >());
    BOOST_CHECK(b1 == b2);
    BOOST_CHECK(!(b1 != b2));

    boost::mpl::map<boost::mpl::pair<_a, int> > m2;
    binding<boost::mpl::vector<> > b3(m2);
    BOOST_CHECK(b3 == b1);
    BOOST_CHECK(!(b3 != b1));
    binding<boost::mpl::vector<> > b4(make_binding<boost::mpl::map<boost::mpl::pair<_a, int> > >());
    BOOST_CHECK(b4 == b1);
    BOOST_CHECK(!(b4 != b1));

    binding<boost::mpl::vector<> > b5(b1, m1);
    BOOST_CHECK(b5 == b1);
    BOOST_CHECK(!(b5 != b1));
    binding<boost::mpl::vector<> > b6(b1, make_binding<boost::mpl::map<> >());
    BOOST_CHECK(b6 == b1);
    BOOST_CHECK(!(b6 != b1));
    
    boost::mpl::map<boost::mpl::pair<_a, _b> > m3;
    binding<boost::mpl::vector<> > b7(b1, m3);
    BOOST_CHECK(b7 == b1);
    BOOST_CHECK(!(b7 != b1));
    binding<boost::mpl::vector<> > b8(b1, make_binding<boost::mpl::map<boost::mpl::pair<_a, _b> > >());
    BOOST_CHECK(b8 == b1);
    BOOST_CHECK(!(b8 != b1));
}

BOOST_AUTO_TEST_CASE(test_binding_one)
{
    boost::mpl::map<boost::mpl::pair<_a, int> > m1;
    binding<typeid_<_a> > b1(m1);
    BOOST_CHECK(b1.find<typeid_<_a> >()() == typeid(int));
    binding<typeid_<_a> > b2(make_binding<boost::mpl::map<boost::mpl::pair<_a, int> > >());
    BOOST_CHECK(b2.find<typeid_<_a> >()() == typeid(int));
    BOOST_CHECK(b1 == b2);
    BOOST_CHECK(!(b1 != b2));

    boost::mpl::map<boost::mpl::pair<_a, _a> > m2;
    binding<typeid_<_a> > b3(b1, m2);
    BOOST_CHECK(b3.find<typeid_<_a> >()() == typeid(int));
    BOOST_CHECK(b3 == b1);
    BOOST_CHECK(!(b3 != b1));
    binding<typeid_<_a> > b4(b1, make_binding<boost::mpl::map<boost::mpl::pair<_a, _a> > >());
    BOOST_CHECK(b4.find<typeid_<_a> >()() == typeid(int));
    BOOST_CHECK(b4 == b1);
    BOOST_CHECK(!(b4 != b1));

    boost::mpl::map<boost::mpl::pair<_b, _a> > m3;
    binding<typeid_<_b> > b5(b1, m3);
    BOOST_CHECK(b5.find<typeid_<_b> >()() == typeid(int));
    binding<typeid_<_b> > b6(b1, make_binding<boost::mpl::map<boost::mpl::pair<_b, _a> > >());
    BOOST_CHECK(b6.find<typeid_<_b> >()() == typeid(int));
}

BOOST_AUTO_TEST_CASE(test_binding_two)
{
    boost::mpl::map<boost::mpl::pair<_a, int>, boost::mpl::pair<_b, char> > m1;
    binding<boost::mpl::vector<typeid_<_a>, typeid_<_b> > > b1(m1);
    BOOST_CHECK(b1.find<typeid_<_a> >()() == typeid(int));
    BOOST_CHECK(b1.find<typeid_<_b> >()() == typeid(char));
    binding<boost::mpl::vector<typeid_<_a>, typeid_<_b> > > b2(
        make_binding<boost::mpl::map<boost::mpl::pair<_a, int>, boost::mpl::pair<_b, char> > >());
    BOOST_CHECK(b2.find<typeid_<_a> >()() == typeid(int));
    BOOST_CHECK(b2.find<typeid_<_b> >()() == typeid(char));
    BOOST_CHECK(b1 == b2);
    BOOST_CHECK(!(b1 != b2));

    // select the first
    boost::mpl::map<boost::mpl::pair<_a, _a> > m2;
    binding<typeid_<_a> > b3(b1, m2);
    BOOST_CHECK(b3.find<typeid_<_a> >()() == typeid(int));
    binding<typeid_<_a> > b4(b1, make_binding<boost::mpl::map<boost::mpl::pair<_a, _a> > >());
    BOOST_CHECK(b4.find<typeid_<_a> >()() == typeid(int));

    // select the second
    boost::mpl::map<boost::mpl::pair<_b, _b> > m3;
    binding<typeid_<_b> > b5(b1, m3);
    BOOST_CHECK(b5.find<typeid_<_b> >()() == typeid(char));
    binding<typeid_<_b> > b6(b1, make_binding<boost::mpl::map<boost::mpl::pair<_b, _b> > >());
    BOOST_CHECK(b6.find<typeid_<_b> >()() == typeid(char));

    // rename both
    boost::mpl::map<boost::mpl::pair<_c, _a>, boost::mpl::pair<_d, _b> > m4;
    binding<boost::mpl::vector<typeid_<_c>, typeid_<_d> > > b7(b1, m4);
    BOOST_CHECK(b7.find<typeid_<_c> >()() == typeid(int));
    BOOST_CHECK(b7.find<typeid_<_d> >()() == typeid(char));
    binding<boost::mpl::vector<typeid_<_c>, typeid_<_d> > > b8(b1,
        make_binding<boost::mpl::map<boost::mpl::pair<_c, _a>, boost::mpl::pair<_d, _b> > >());
    BOOST_CHECK(b8.find<typeid_<_c> >()() == typeid(int));
    BOOST_CHECK(b8.find<typeid_<_d> >()() == typeid(char));

    // switch the placeholders
    boost::mpl::map<boost::mpl::pair<_a, _b>, boost::mpl::pair<_b, _a> > m5;
    binding<boost::mpl::vector<typeid_<_a>, typeid_<_b> > > b9(b1, m5);
    BOOST_CHECK(b9.find<typeid_<_b> >()() == typeid(int));
    BOOST_CHECK(b9.find<typeid_<_a> >()() == typeid(char));
    binding<boost::mpl::vector<typeid_<_a>, typeid_<_b> > > b10(b1,
        make_binding<boost::mpl::map<boost::mpl::pair<_a, _b>, boost::mpl::pair<_b, _a> > >());
    BOOST_CHECK(b10.find<typeid_<_b> >()() == typeid(int));
    BOOST_CHECK(b10.find<typeid_<_a> >()() == typeid(char));
}
