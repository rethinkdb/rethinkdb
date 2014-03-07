//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2011 Dean Michael Berries
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/lightweight_test.hpp>
#include <boost/config/warning_disable.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/tuple.hpp>
#include <string>

struct foo_parts 
{
    boost::optional<std::string> first;
    std::string second;
};

namespace boost { namespace spirit { namespace traits 
{
    template <>
    struct transform_attribute<foo_parts
      , fusion::tuple<std::string &, optional<std::string> &>
      , spirit::qi::domain>
    {
        typedef fusion::tuple<std::string&, optional<std::string>&> type;

        static type pre(foo_parts & parts) 
        {
            return fusion::tie(parts.second, parts.first);
        }

        static void post(foo_parts &, type const &) {}
        static void fail(foo_parts &) {}
    };
}}}    

namespace qi = boost::spirit::qi;

template <typename Iterator>
struct foo_grammar : qi::grammar<Iterator, foo_parts()> 
{
    foo_grammar() : foo_grammar::base_type(start, "foo") 
    {
        foo_part = 
               +qi::alpha >> -(+qi::digit)
            |   qi::attr(std::string()) 
                >> qi::attr(boost::optional<std::string>())
            ;

        start = foo_part.alias();
    }

    typedef boost::fusion::tuple<std::string&, boost::optional<std::string>&>
        tuple_type;

    qi::rule<Iterator, tuple_type()> foo_part;
    qi::rule<Iterator, foo_parts()> start;
};

int main() 
{
    foo_parts instance;
    foo_grammar<std::string::iterator> grammar;
    std::string input = "abc123";
    
    BOOST_TEST(qi::parse(input.begin(), input.end(), grammar, instance) &&
        instance.first && instance.first.get() == "123" && 
        instance.second == "abc");

    return boost::report_errors();
}

