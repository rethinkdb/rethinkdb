//  Copyright (c) 2011 Roji Philip
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/fusion/include/adapt_adt.hpp>
#include <boost/optional.hpp>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_adapt_adt_attributes.hpp>

#include "test.hpp"

///////////////////////////////////////////////////////////////////////////////
struct test1 
{
    unsigned var;
    boost::optional<unsigned> opt;

    unsigned& getvar() { return var; }
    unsigned const& getvar() const { return var; }
    void setvar(unsigned val) { var = val; }

    boost::optional<unsigned>& getopt() { return opt; }
    boost::optional<unsigned> const& getopt() const { return opt; }
    void setopt(boost::optional<unsigned> const& val) { opt = val; }
};

BOOST_FUSION_ADAPT_ADT(
    test1,
    (unsigned&, unsigned const&, obj.getvar(), obj.setvar(val))
    (boost::optional<unsigned>&, boost::optional<unsigned> const&, 
        obj.getopt(), obj.setopt(val))
)

///////////////////////////////////////////////////////////////////////////////
struct test2
{
    std::string str;
    boost::optional<std::string> optstr;

    std::string& getstring() { return str; }
    std::string const& getstring() const { return str; }
    void setstring(std::string const& val) { str = val; }

    boost::optional<std::string>& getoptstring() { return optstr; }
    boost::optional<std::string> const& getoptstring() const { return optstr; }
    void setoptstring(boost::optional<std::string> const& val) { optstr = val; }
};

BOOST_FUSION_ADAPT_ADT(
    test2,
    (std::string&, std::string const&, obj.getstring(), obj.setstring(val))
    (boost::optional<std::string>&, boost::optional<std::string> const&, 
        obj.getoptstring(), obj.setoptstring(val))
)

///////////////////////////////////////////////////////////////////////////////
int main() 
{
    using spirit_test::test_attr;
    namespace qi = boost::spirit::qi;

    {
        test1 data;
        BOOST_TEST(test_attr("123@999", qi::uint_ >> -('@' >> qi::uint_), data) &&
            data.var == 123 && data.opt && data.opt.get() == 999);
    }

    {
        test1 data;
        BOOST_TEST(test_attr("123", qi::uint_ >> -('@' >> qi::uint_), data) &&
            data.var == 123 && !data.opt);
    }

    {
        test2 data;
        BOOST_TEST(test_attr("Hello:OptionalHello", 
            +qi::alnum >> -(':' >> +qi::alnum), data) &&
            data.str == "Hello" && 
            data.optstr && data.optstr.get() == "OptionalHello");
    }

    {
        test2 data;
        BOOST_TEST(test_attr("Hello", 
            +qi::alnum >> -(':' >> +qi::alnum), data) &&
            data.str == "Hello" && !data.optstr);
    }

    return boost::report_errors();
}
