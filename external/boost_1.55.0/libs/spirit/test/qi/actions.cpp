/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if defined(_MSC_VER)
# pragma warning(disable: 4180)     // qualifier applied to function type
                                    // has no meaning; ignored
#endif

#include <boost/detail/lightweight_test.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/spirit/include/qi_operator.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_action.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>
#include <cstring>

int x = 0;

void fun1(int const& i)
{
    x += i;
}

void fun2(int i)
{
    x += i;
}
using boost::spirit::unused_type;

struct fun_action
{
    void operator()(int const& i, unused_type, unused_type) const
    {
        x += i;
    }
};

void fail (int, boost::spirit::unused_type, bool& pass)
{ 
    pass = false; 
} 

struct setnext
{
    setnext(char& next) : next(next) {}

    void operator()(char c, unused_type, unused_type) const
    {
        next = c;
    }

    char& next;
};

int main()
{
    namespace qi = boost::spirit::qi;
    using boost::spirit::int_;

    {
        char const *s1 = "{42}", *e1 = s1 + std::strlen(s1);
        qi::parse(s1, e1, '{' >> int_[&fun1] >> '}');
    }

    {
        char const *s1 = "{42}", *e1 = s1 + std::strlen(s1);
        qi::parse(s1, e1, '{' >> int_[&fun2] >> '}');
    }

#if !BOOST_WORKAROUND(BOOST_MSVC, <= 1400)
    {
        char const *s1 = "{42}", *e1 = s1 + std::strlen(s1);
        qi::parse(s1, e1, '{' >> int_[fun2] >> '}');
    }
#else
    x += 42;        // compensate for missing test case
#endif

    {
        char const *s1 = "{42}", *e1 = s1 + std::strlen(s1);
        qi::parse(s1, e1, '{' >> int_[fun_action()] >> '}');
    }

    {
        char const *s1 = "{42}", *e1 = s1 + std::strlen(s1);
        qi::parse(s1, e1, '{' >> int_[boost::bind(&fun1, _1)] >> '}');
    }

    {
        namespace lambda = boost::lambda;
        char const *s1 = "{42}", *e1 = s1 + std::strlen(s1);
        qi::parse(s1, e1, '{' >> int_[lambda::var(x) += lambda::_1] >> '}');
    }
    BOOST_TEST(x == (42*6));

    {
       std::string input("1234 6543"); 
       char next = '\0';
       BOOST_TEST(qi::phrase_parse(input.begin(), input.end(),
          qi::int_[fail] | qi::digit[setnext(next)] , qi::space));
       BOOST_TEST(next == '1'); 
    }

    return boost::report_errors();
}




