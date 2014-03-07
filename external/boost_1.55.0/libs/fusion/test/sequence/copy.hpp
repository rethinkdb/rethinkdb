/*=============================================================================
    Copyright (c) 1999-2003 Jaakko Jarvi
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/mpl/insert_range.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/static_assert.hpp>

#if !defined(FUSION_AT)
#define FUSION_AT at_c
#endif

#if !defined(FUSION_MAKE)
#define FUSION_MAKE BOOST_PP_CAT(make_, FUSION_SEQUENCE)
#endif

#if !defined(FUSION_TIE)
#define FUSION_TIE BOOST_PP_CAT(FUSION_SEQUENCE, _tie)
#endif

namespace test_detail
{
    // classes with different kinds of conversions
    class AA {};
    class BB : public AA {};
    struct CC { CC() {} CC(const BB&) {} };
    struct DD { operator CC() const { return CC(); }; };
}

boost::fusion::FUSION_SEQUENCE<double, double, double, double>
foo(int i)
{
   return boost::fusion::FUSION_MAKE(i, i+1, i+2, i+3);
}

void test_mpl()
{
    using namespace boost::fusion;

    typedef FUSION_SEQUENCE<int, char> seq;

    typedef
        boost::mpl::insert_range<
            boost::mpl::vector<>
          , boost::mpl::end< boost::mpl::vector<> >::type
          , seq
        >::type
    sequence;

    typedef boost::mpl::equal<sequence, boost::mpl::vector<int, char> > equal;
    BOOST_STATIC_ASSERT(equal::value);
}

void
test()
{
    using namespace boost::fusion;
    using namespace test_detail;

    FUSION_SEQUENCE<int, char> t1(4, 'a');
    FUSION_SEQUENCE<int, char> t2(5, 'b');
    t2 = t1;
    BOOST_TEST(FUSION_AT<0>(t1) == FUSION_AT<0>(t2));
    BOOST_TEST(FUSION_AT<1>(t1) == FUSION_AT<1>(t2));

    FUSION_SEQUENCE<long, std::string> t3(2, "a");
    t3 = t1;
    BOOST_TEST((double)FUSION_AT<0>(t1) == FUSION_AT<0>(t3));
    BOOST_TEST(FUSION_AT<1>(t1) == FUSION_AT<1>(t3)[0]);

    // testing copy and assignment with implicit conversions
    // between elements testing tie

    FUSION_SEQUENCE<char, BB*, BB, DD> t;
    FUSION_SEQUENCE<int, AA*, CC, CC> a(t);
    a = t;

    int i; char c; double d;
    FUSION_TIE(i, c, d) = FUSION_MAKE(1, 'a', 5.5);

    BOOST_TEST(i==1);
    BOOST_TEST(c=='a');
    BOOST_TEST(d>5.4 && d<5.6);

    // returning a tuple with conversion
    foo(2);

    test_mpl();
}
