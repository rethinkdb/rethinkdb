//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #define KARMA_TEST_COMPILE_FAIL

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_operator.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_action.hpp>
#include <boost/spirit/include/karma_nonterminal.hpp>
#include <boost/spirit/include/karma_auxiliary.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/support_unused.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/vector.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
// lazy version of fusion::size
struct seqsize_impl
{
    template <typename Sequence>
    struct result
      : boost::fusion::result_of::size<Sequence>
    {};

    template <typename This, typename Sequence>
    struct result<This(Sequence)>
        : result<typename boost::proto::detail::uncvref<Sequence>::type>
    {};

    template <typename Sequence>
    typename result<Sequence>::type
    operator()(Sequence const& seq) const
    {
        return boost::fusion::size(seq);
    }
};

boost::phoenix::function<seqsize_impl> const seqsize = seqsize_impl();

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;
    namespace fusion = boost::fusion;

    {
        std::list<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        BOOST_TEST(test("123", int_ << int_ << int_, v));
        BOOST_TEST(test_delimited("1 2 3 ", int_ << int_ << int_, v, ' '));
        BOOST_TEST(test("1,2,3", int_ << ',' << int_ << ',' << int_, v));
        BOOST_TEST(test_delimited("1 , 2 , 3 ", int_ << ',' << int_ << ',' << int_, v, ' '));
    }

    {
        BOOST_TEST(test("aa", lower[char_('A') << 'a']));
        BOOST_TEST(test_delimited("BEGIN END ", 
            upper[lit("begin") << "end"], char(' ')));
        BOOST_TEST(!test_delimited("BEGIN END ", 
            upper[lit("begin") << "nend"], char(' ')));

        BOOST_TEST(test("Aa        ", left_align[char_('A') << 'a']));
        BOOST_TEST(test("    Aa    ", center[char_('A') << 'a']));
        BOOST_TEST(test("        Aa", right_align[char_('A') << 'a']));
    }

    {
        // make sure single element tuples get passed through if the rhs 
        // has a single element tuple as its attribute
        typedef spirit_test::output_iterator<char>::type iterator_type;
        fusion::vector<double, int> fv(2.0, 1);
        karma::rule<iterator_type, fusion::vector<double, int>()> r;
        r %= double_ << ',' << int_;
        BOOST_TEST(test("test:2.0,1", "test:" << r, fv));
    }

    // action tests
    {
        using namespace boost::phoenix;

        BOOST_TEST(test("abcdefg", 
            (char_ << char_ << string)[_1 = 'a', _2 = 'b', _3 = "cdefg"]));
        BOOST_TEST(test_delimited("a b cdefg ", 
            (char_ << char_ << string)[_1 = 'a', _2 = 'b', _3 = "cdefg"], 
            char(' ')));

        BOOST_TEST(test_delimited("a 12 c ", 
            (char_ << lit(12) << char_)[_1 = 'a', _2 = 'c'], char(' ')));

        char c = 'c';
        BOOST_TEST(test("abc", 
            (char_[_1 = 'a'] << 'b' << char_)[_1 = 'x', _2 = ref(c)]));
        BOOST_TEST(test_delimited("a b c ", 
            (char_[_1 = 'a'] << 'b' << char_)[_2 = ref(c)], char(' ')));

        BOOST_TEST(test("aa", lower[char_ << 'A'][_1 = 'A']));
        BOOST_TEST(test("AA", upper[char_ << 'a'][_1 = 'a']));

        BOOST_TEST(test("Aa        ", left_align[char_ << 'a'][_1 = 'A']));
        BOOST_TEST(test("    Aa    ", center[char_ << 'a'][_1 = 'A']));
        BOOST_TEST(test("        Aa", right_align[char_ << 'a'][_1 = 'A']));
    }

    // test special case where sequence has a one element vector attribute 
    // sequence and this element is a rule (attribute has to be passed through 
    // without change)
    {
        typedef spirit_test::output_iterator<char>::type outiter_type;
        namespace karma = boost::spirit::karma;

        karma::rule<outiter_type, std::vector<int>()> r = -(int_ % ',');
        std::vector<int> v;
        BOOST_TEST(test(">", '>' << r, v));

        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.push_back(4);
        BOOST_TEST(test(">1,2,3,4", '>' << r, v));
    }

    {
        namespace karma = boost::spirit::karma;
        typedef spirit_test::output_iterator<char>::type outiter_type;

        karma::rule<outiter_type, std::string()> e = karma::string;
        karma::rule<outiter_type, std::vector<std::string>()> l = e << *(',' << e);

        std::vector<std::string> v;
        v.push_back("abc1");
        v.push_back("abc2");
        v.push_back("abc3");
        BOOST_TEST(test("abc1,abc2,abc3", l, v));
    }

    {
        namespace karma = boost::spirit::karma;
        namespace phoenix = boost::phoenix;

        typedef spirit_test::output_iterator<char>::type outiter_type;
        typedef fusion::vector<char, char, char> vector_type;
        
        vector_type p ('a', 'b', 'c');
        BOOST_TEST(test("ab", char_ << char_, p));

        karma::rule<outiter_type, vector_type()> r;
        r %= char_ << char_ << &karma::eps[seqsize(_val) == 3];
        BOOST_TEST(!test("", r, p));

        r %= char_ << char_ << char_ << &karma::eps[seqsize(_val) == 3];
        BOOST_TEST(test("abc", r, p));
    }

    {
        std::list<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.push_back(4);

        BOOST_TEST(test("1234", repeat(2)[int_] << *int_, v));
        BOOST_TEST(test_delimited("1 2 3 4 ", repeat(2)[int_] << *int_, v, char(' ')));
    }

    return boost::report_errors();
}

