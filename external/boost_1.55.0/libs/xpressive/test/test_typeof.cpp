///////////////////////////////////////////////////////////////////////////////
// test_typeof.cpp
//
//  Copyright 2008 David Jenkins. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TYPEOF_LIMIT_SIZE 200
#define BOOST_TYPEOF_EMULATION 1

#include <string>
#include <boost/version.hpp>
#include <boost/xpressive/xpressive_static.hpp>
#include <boost/xpressive/xpressive_typeof.hpp>
#include <boost/test/unit_test.hpp>

// I couldn't find these registrations anywhere else, so I put them here
// They are necessary for this program to compile
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::mpl::int_, (int))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::reference_wrapper, (typename))

// Here's the test for typeof registration, to be used on static regular expressions
#define TYPEOF_TEST(Expr) { BOOST_PROTO_AUTO(Dummy, Expr); }

namespace xp = boost::xpressive;

///////////////////////////////////////////////////////////////////////////////
// test_misc1
//  miscelaneous regular expressions
static void test_misc1()
{
    using namespace boost::xpressive;

    TYPEOF_TEST(epsilon);
    TYPEOF_TEST(nil);
    TYPEOF_TEST(alnum);
    TYPEOF_TEST(bos);
    TYPEOF_TEST(eos);
    TYPEOF_TEST(bol);
    TYPEOF_TEST(eol);
    TYPEOF_TEST(bow);
    TYPEOF_TEST(eow);
    TYPEOF_TEST(_b);
    TYPEOF_TEST(_w);
    TYPEOF_TEST(_d);
    TYPEOF_TEST(_s);
    TYPEOF_TEST(_n);
    TYPEOF_TEST(_ln);
    TYPEOF_TEST(_);
    TYPEOF_TEST(self);
}

///////////////////////////////////////////////////////////////////////////////
// test_misc2
//  miscelaneous regular expressions
static void test_misc2()
{
    using namespace boost::xpressive;

    TYPEOF_TEST(+set[_d | '-' | 'g']);
    TYPEOF_TEST(+set['g' | as_xpr('-') | _d]);
    TYPEOF_TEST(icase(+range('a','g')));
    TYPEOF_TEST(+range('-','/'));
    TYPEOF_TEST(+range('+','-'));
    TYPEOF_TEST(+range('b','b'));
    TYPEOF_TEST(icase((s1= "foo") >> *_ >> '\15'));
    TYPEOF_TEST(repeat<2>(repeat<3>(_d) >> '-') >> repeat<4>(_d));
    TYPEOF_TEST('f' >> +as_xpr('o'));
    TYPEOF_TEST(icase(+(s1= "foo") >> "foobar"));
    cregex parens = '(' >> *( keep( +~(set='(',')') ) | by_ref(parens) ) >> ')';
    TYPEOF_TEST(_b >> "sublist" >> parens);
    TYPEOF_TEST(bos >> "baz" | "bar");
    TYPEOF_TEST(icase(*_ >> "foo"));
    TYPEOF_TEST(icase(*_ >> "boo" | "bar"));
    TYPEOF_TEST(icase("bar"));

    TYPEOF_TEST(('f' >> repeat<1,repeat_max>('o')));
    TYPEOF_TEST("This " >> (s1= "(has)") >> ' ' >> (s2= "parens"));
    TYPEOF_TEST(as_xpr("This (has parens"));
    TYPEOF_TEST(+_d);
    TYPEOF_TEST(+~_d);
    TYPEOF_TEST(+set[_d]);
    TYPEOF_TEST(+set[~_d]);
    TYPEOF_TEST(+~set[~_d]);
    TYPEOF_TEST(+~set[_d]);
    TYPEOF_TEST(+set[~_w | ~_d]);
    TYPEOF_TEST(+~set[_w | _d]);
    TYPEOF_TEST((s1= '(' >> (s2= nil) | '[' >> (s3= nil)) >> -*_ >> (s4= ')') >> s2 | ']' >> s3);
    TYPEOF_TEST(after("foo") >> "bar");
    TYPEOF_TEST(after(s1= 'f' >> _ >> 'o') >> "bar");
    TYPEOF_TEST(icase(after(s1= "fo") >> 'o'));
    TYPEOF_TEST(icase(~after(s1= "fo") >> 'o'));
    TYPEOF_TEST(+alpha);
    TYPEOF_TEST(+set[alpha | digit]);
    TYPEOF_TEST(after(s1= nil) >> 'a');
    TYPEOF_TEST(after(s1= "abc" >> repeat<3>(_d)) >> "foo");
    TYPEOF_TEST(~before(bol) >> 'x');
    TYPEOF_TEST(~before(bos) >> 'x');
}

///////////////////////////////////////////////////////////////////////////////
// test_misc3
//  miscelaneous regular expressions

static void test_misc3()
{
    using namespace boost::xpressive;

    TYPEOF_TEST(as_xpr("foo"));
    TYPEOF_TEST('b' >> *_ >> "ar");
    TYPEOF_TEST('b' >> *_ >> 'r');
    TYPEOF_TEST('b' >> +_ >> "ar");
    TYPEOF_TEST('b' >> +_ >> 'r');
    TYPEOF_TEST('b' >> +_ >> "oo");
    TYPEOF_TEST(bos >> "foo");
    TYPEOF_TEST(bos >> 'b' >> *_ >> "ar");
    TYPEOF_TEST('b' >> +_ >> "ar" >> eos);
    TYPEOF_TEST('b' >> +_ >> 'o' >> eos);
    TYPEOF_TEST(bos >> (s1= !(set='-','+') >> +range('0','9')
                           >> !(s2= '.' >> *range('0','9')))
                           >> (s3= (set='C','F')) >> eos);
    TYPEOF_TEST( !(s1= as_xpr('+')|'-') >> (s2= +range('0','9') >> !as_xpr('.') >> *range('0','9') |
                        '.' >> +range('0','9')) >> !(s3= (set='e','E') >> !(s4= as_xpr('+')|'-') >> +range('0','9')));
    TYPEOF_TEST('f' | icase('g'));
    TYPEOF_TEST(icase(+lower));
    TYPEOF_TEST(icase(+as_xpr('\x61')));
    TYPEOF_TEST(icase(+set['\x61']));
    TYPEOF_TEST(icase(+as_xpr('\x0061')));
    TYPEOF_TEST(icase(+set['\x0061']));
    TYPEOF_TEST('a' >> +(s1= 'b' | (s2= *(s3= 'c'))) >> 'd');
    TYPEOF_TEST('a' >> +(s1= 'b' | (s2= !(s3= 'c'))) >> 'd');
    TYPEOF_TEST(*as_xpr('a') >> *as_xpr('a') >> *as_xpr('a') >> *as_xpr('a') >> *as_xpr('a') >> 'b');
    TYPEOF_TEST(*set[range('a','z') | range('A','Z')]);
}

///////////////////////////////////////////////////////////////////////////////
// test_misc4
//  miscelaneous regular expressions
static void test_misc4()
{
    using namespace boost::xpressive;
    TYPEOF_TEST('a' >> bos >> 'b');
    TYPEOF_TEST(as_xpr("a^b"));
    TYPEOF_TEST('a' >> ~set[' '] >> 'b');
    TYPEOF_TEST('a' >> ~set['^'] >> 'b');
    TYPEOF_TEST('a' >> ~set['^'] >> 'b');
    TYPEOF_TEST('a' >> set['^'] >> 'b');
    TYPEOF_TEST(icase("foo" >> before("bar")));
    TYPEOF_TEST(icase("foo" >> ~before("bar")));
    TYPEOF_TEST(icase("foo" >> ~before("bar")));
    TYPEOF_TEST(icase(+(s1= keep(s2= "foo") >> "bar")));
    TYPEOF_TEST(+(s1= "bar" | (s2= "foo")));
    TYPEOF_TEST(+(s1= (s2= "bar") | "foo"));
    TYPEOF_TEST(+(s1= "foo" | (s2= "bar")));
    TYPEOF_TEST(+(s1= (s2= "foo") | "bar"));
    TYPEOF_TEST((s1= icase("FOO")) >> (s2= -*_) >> s1);
    TYPEOF_TEST((s1= icase("FOO")) >> (s2= -*_) >> icase(s1));
    TYPEOF_TEST(+(s1= "foo" | icase(s1 >> 'O')));
    TYPEOF_TEST((bos >> set[range('A','Z') | range('a','m')]));
    TYPEOF_TEST(('f' >> repeat<2,5>('o')));
    TYPEOF_TEST(('f' >> -repeat<2,5>('o')));
    TYPEOF_TEST(('f' >> repeat<2,5>('o') >> 'o'));
    TYPEOF_TEST(('f' >> -repeat<2,5>('o') >> 'o'));
    TYPEOF_TEST(bos >> '{' >> *_ >> '}' >> eos);
    TYPEOF_TEST(+(set='+','-'));
    TYPEOF_TEST(+(set='-','+'));
}

///////////////////////////////////////////////////////////////////////////////
// test_misc5
//  miscelaneous regular expressions
static void test_misc5()
{
    using namespace boost::xpressive;
    TYPEOF_TEST(bos >> ('(' >> (s1= nil) | (s2= nil)) >> +_w >> (')' >> s1 | s2) >> eos);
    TYPEOF_TEST(+~alpha);
    TYPEOF_TEST(+set[alpha | ~alpha]);
    TYPEOF_TEST(+~set[~alpha]);
    TYPEOF_TEST(as_xpr("[[:alpha:]\\y]+"));
    TYPEOF_TEST(+~set[~alnum | ~digit]);
    TYPEOF_TEST(icase(bos >> repeat<4>(s1= 'a' >> !s1) >> eos));
    TYPEOF_TEST(as_xpr("foo") >> /*This is a comment[*/ "bar");
    TYPEOF_TEST(bos >> "foobar" >> eos);
    TYPEOF_TEST(bos >> 'f' >> *as_xpr('o'));
    TYPEOF_TEST(bos >> 'f' >> *as_xpr('\157'));
    TYPEOF_TEST(bos >> ("foo" >> set[' '] >> "bar") >> eos /*This is a comment*/);
}

///////////////////////////////////////////////////////////////////////////////
// test_misc6
//  miscelaneous regular expressions
static void test_misc6()
{
    using namespace boost::xpressive;
    TYPEOF_TEST(bos >> *(s1= optional('a')) >> eos);
    TYPEOF_TEST(bos >> -*(s1= optional('a')) >> eos);
    TYPEOF_TEST(bos >> repeat<2>(s1= optional('b')) >> "bc" >> eos);
    TYPEOF_TEST(bos >> *(s1= optional('b')) >> 'd' >> eos);
    TYPEOF_TEST(bos >> -repeat<2>(s1= optional('b')) >> "bc" >> eos);
    TYPEOF_TEST(bos >> -*(s1= optional('b')) >> 'd' >> eos);
    TYPEOF_TEST(bos >> repeat<2>(s1= -optional('b')) >> "bc" >> eos);
    TYPEOF_TEST(bos >> *(s1= -optional('b')) >> 'd' >> eos);
    TYPEOF_TEST(bos >> -repeat<2>(s1= -optional('b')) >> "bc" >> eos);
    TYPEOF_TEST(bos >> -*(s1= -optional('b')) >> 'd' >> eos);
    TYPEOF_TEST(bos >> *(s1= nil | nil | nil | 'b') >> "bc" >> eos);
    TYPEOF_TEST(bos >> -*(s1= nil | nil | nil | 'b') >> "bc" >> eos);
    TYPEOF_TEST(icase(+range('Z','a')));
    TYPEOF_TEST(+range('Z','a'));
}

// These functions are defined in test_typeof2.cpp
void test_actions();
void test_symbols();
void test_assert();

using namespace boost::unit_test;

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test_typeof");
    test->add(BOOST_TEST_CASE(&test_misc1));
    test->add(BOOST_TEST_CASE(&test_misc2));
    test->add(BOOST_TEST_CASE(&test_misc3));
    test->add(BOOST_TEST_CASE(&test_misc4));
    test->add(BOOST_TEST_CASE(&test_misc5));
    test->add(BOOST_TEST_CASE(&test_misc6));
    test->add(BOOST_TEST_CASE(&test_actions));
    test->add(BOOST_TEST_CASE(&test_symbols));
    test->add(BOOST_TEST_CASE(&test_assert));
    return test;
}
