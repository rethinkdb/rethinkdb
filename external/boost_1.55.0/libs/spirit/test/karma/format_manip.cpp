//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/mpl/print.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/karma_format.hpp>
#include <boost/spirit/include/karma_format_auto.hpp>
#include <boost/spirit/include/karma_stream.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include <string>
#include <sstream>
#include <vector>
#include <list>

#include <boost/detail/lightweight_test.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign/std/list.hpp>

///////////////////////////////////////////////////////////////////////////////
template <typename Char, typename Expr>
bool test(Char const *expected, Expr const& xpr)
{
    // Report invalid expression error as early as possible.
    // If you got an error_invalid_expression error message here,
    // then the expression (expr) is not a valid spirit karma expression.
    BOOST_SPIRIT_ASSERT_MATCH(boost::spirit::karma::domain, Expr);

    std::ostringstream ostrm;
    ostrm << boost::spirit::compile<boost::spirit::karma::domain>(xpr);
    return ostrm.good() && ostrm.str() == expected;
}

template <typename Char, typename Expr, typename CopyExpr, typename CopyAttr
  , typename Delimiter, typename Attribute>
bool test(Char const *expected, 
    boost::spirit::karma::detail::format_manip<
        Expr, CopyExpr, CopyAttr, Delimiter, Attribute> const& fm)
{
    std::ostringstream ostrm;
    ostrm << fm;
    return ostrm.good() && ostrm.str() == expected;
}

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost::spirit;
    using namespace boost::spirit::ascii;

    namespace fusion = boost::fusion;
    using namespace boost::phoenix;

    {
        BOOST_TEST(test( "a", 
            char_('a')
        ));
        BOOST_TEST(test( "a", 
            char_[_1 = val('a')]
        ));
        BOOST_TEST(test( "a", 
            karma::format(char_[_1 = val('a')]) 
        ));
        BOOST_TEST(test( "a ", 
            karma::format_delimited(char_[_1 = val('a')], space) 
        ));
        BOOST_TEST(test( "a", 
            karma::format(char_, 'a') 
        ));
        BOOST_TEST(test( "a ", 
            karma::format_delimited(char_, space, 'a') 
        ));
    }

    {
        BOOST_TEST(test( "ab", 
            char_[_1 = val('a')] << char_[_1 = val('b')] 
        ));
        BOOST_TEST(test( "ab", 
            karma::format(char_[_1 = val('a')] << char_[_1 = val('b')]) 
        ));
        BOOST_TEST(test( "a b ", 
            karma::format_delimited(char_[_1 = val('a')] << char_[_1 = val('b')], space) 
        ));

        fusion::vector<char, char> t('a', 'b');

        BOOST_TEST(test( "ab", 
            karma::format(char_ << char_, t) 
        ));
        BOOST_TEST(test( "a b ", 
            karma::format_delimited(char_ << char_, space, t) 
        ));

        BOOST_TEST(test( "ab", 
            karma::format(t) 
        ));
        BOOST_TEST(test( "a b ", 
            karma::format_delimited(t, space) 
        ));
    }

    {
        BOOST_TEST(test( "abc", 
            char_[_1 = 'a'] << char_[_1 = 'b'] << char_[_1 = 'c']
        ));
        BOOST_TEST(test( "abc", 
            karma::format(char_('a') << char_('b') << char_('c')) 
        ));
        BOOST_TEST(test( "a b c ", 
            karma::format_delimited(char_('a') << char_('b') << char_('c'), space) 
        ));

        fusion::vector<char, char, char> t('a', 'b', 'c');

        BOOST_TEST(test( "abc", 
            karma::format(char_ << char_ << char_, t) 
        ));
        BOOST_TEST(test( "a b c ", 
            karma::format_delimited(char_ << char_ << char_, space, t) 
        ));
    }

    {
        BOOST_TEST(test( "a2", 
            (char_ << int_)[_1 = 'a', _2 = 2] 
        ));

        fusion::vector<char, int> t('a', 2);

        BOOST_TEST(test( "a2", 
            karma::format(char_ << int_, t) 
        ));
        BOOST_TEST(test( "a 2 ", 
            karma::format_delimited(char_ << int_, space, t) 
        ));
    }

    using namespace boost::assign;

    {
        // output all elements of a vector
        std::vector<char> v;
        v += 'a', 'b', 'c';

        BOOST_TEST(test( "abc", 
            (*char_)[_1 = v] 
        ));
        BOOST_TEST(test( "abc", 
            karma::format(*char_, v)
        ));
        BOOST_TEST(test( "a b c ", 
            karma::format_delimited(*char_, space, v)
        ));

        BOOST_TEST(test( "abc", 
            karma::format(v)
        ));
        BOOST_TEST(test( "a b c ", 
            karma::format_delimited(v, space)
        ));

        // output a comma separated list of vector elements
        BOOST_TEST(test( "a, b, c", 
            (char_ % lit(", "))[_0 = fusion::make_single_view(v)] 
        ));
        BOOST_TEST(test( "a, b, c", 
            karma::format((char_ % lit(", "))[_0 = fusion::make_single_view(v)])
        ));
        BOOST_TEST(test( "a , b , c ", 
            karma::format_delimited((char_ % ',')[_0 = fusion::make_single_view(v)], space)
        ));
        BOOST_TEST(test( "a,b,c", 
            karma::format(char_ % ',', v)
        ));
        BOOST_TEST(test( "a , b , c ", 
            karma::format_delimited(char_ % ',', space, v)
        ));

        // output all elements of a list
        std::list<char> l;
        l += 'a', 'b', 'c';

//         BOOST_TEST(test( "abc", 
//             (*char_)[_1 = l] 
//         ));
//         BOOST_TEST(test( "abc", 
//             karma::format((*char_)[_1 = l])
//         ));
//         BOOST_TEST(test( "a b c ", 
//             karma::format_delimited((*char_)[_1 = l], space)
//         ));
        BOOST_TEST(test( "abc", 
            karma::format(*char_, l)
        ));
        BOOST_TEST(test( "a b c ", 
            karma::format_delimited(*char_, space, l)
        ));

        BOOST_TEST(test( "abc", 
            karma::format(l)
        ));
        BOOST_TEST(test( "a b c ", 
            karma::format_delimited(l, space)
        ));
    }

    return boost::report_errors();
}

