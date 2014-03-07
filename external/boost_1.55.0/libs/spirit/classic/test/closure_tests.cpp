/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <boost/detail/lightweight_test.hpp>

using namespace std;

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_closure.hpp>
#include <boost/spirit/include/classic_parametric.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;

///////////////////////////////////////////////////////////////////////////////
//
//  Closure tests
//
///////////////////////////////////////////////////////////////////////////////
struct my_closure1 : BOOST_SPIRIT_CLASSIC_NS::closure<my_closure1, double>
{
    member1 val;
};

struct my_closure2 : BOOST_SPIRIT_CLASSIC_NS::closure<my_closure2, char>
{
    member1 ch;
};

struct my_closure3 : BOOST_SPIRIT_CLASSIC_NS::closure<my_closure3, char>
{
    member1 ch;
};

struct X { int a; int b; };

#if defined(BOOST_SPIRIT_DEBUG)
//  If debugging is switched on, all closure members should have a 
//  corresponding output streaming operator
std::ostream &
operator<< (std::ostream& o, X const &x)
{
    o << "X(" << x.a << ", " << x.b << ")";
    return o;
}
#endif // defined(BOOST_SPIRIT_DEBUG)

struct my_closure4 : BOOST_SPIRIT_CLASSIC_NS::closure<my_closure4, X>
{
    member1 x;
};

// MWCW8.3 needs the default constructor here or it won't compile.
// It should not be needed.
struct Y { Y() {} Y(int) {} };

#if defined(BOOST_SPIRIT_DEBUG)
//  If debugging is switched on, all closure members should have a 
//  corresponding output streaming operator
std::ostream &
operator<< (std::ostream& o, Y const &x)
{
    o << "Y";
    return o;
}
#endif // defined(BOOST_SPIRIT_DEBUG)

struct my_closure5 : BOOST_SPIRIT_CLASSIC_NS::closure<my_closure5, int, Y>
{
    member1 y;
};

struct my_closure6 : BOOST_SPIRIT_CLASSIC_NS::closure<my_closure6, int, int, int>
{
    member1 x;
    member2 y;
    member3 z;
};

void
closure_tests()
{
    rule<phrase_scanner_t, my_closure1::context_t> num_list;
    double n;

    num_list =
    (
        real_p[num_list.val = arg1] >>  *(',' >> real_p[num_list.val += arg1])
    )
    [var(n) = num_list.val];

    parse_info<char const*> pi;
    pi = parse("123, 456, 789", num_list, space_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);
    BOOST_TEST(n == 123 + 456 + 789);

    rule<scanner<>, my_closure2::context_t> rev;
    rev = anychar_p[rev.ch = arg1] >> !rev >> f_ch_p(rev.ch);

    pi = parse("xyzzyx", rev);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("xyzczyx", rev);
    BOOST_TEST(!pi.hit);

    subrule<0, my_closure3::context_t> rev2;
    pi = parse("atoyyota",
        rev2 = anychar_p[rev2.ch = arg1] >> !rev2 >> f_ch_p(rev2.ch)
    );

    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    pi = parse("whatdahell",
        rev2 = anychar_p[rev2.ch = arg1] >> !rev2 >> f_ch_p(rev2.ch)
    );
    BOOST_TEST(!pi.hit);

    rule<phrase_scanner_t, my_closure4::context_t> complex_p;
    complex_p =
            int_p[bind(&X::a)(complex_p.x) = arg1]
        >>  ','
        >>  int_p[bind(&X::b)(complex_p.x) = arg1]
    ;

    X x;
    pi = parse("123, 456", complex_p[var(x) = arg1], space_p);
    BOOST_TEST(pi.hit);
    BOOST_TEST(x.a == 123);
    BOOST_TEST(x.b == 456);

    rule<scanner<>, my_closure5::context_t> init1; // compile check only
    rule<> r1 = init1(3, 3); // member2 is constructed from int

    rule<scanner<>, my_closure6::context_t> init2; // compile check only
    rule<> r2 = init2(3); // member2 and member3 are default constructed
}

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
int
main()
{
    closure_tests();
    return boost::report_errors();
}

