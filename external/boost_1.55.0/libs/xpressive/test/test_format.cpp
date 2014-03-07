///////////////////////////////////////////////////////////////////////////////
// test_format.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  Test all the different ways to call regex_replace with a formatter.

#include <map>
#include <string>
#include <boost/test/unit_test.hpp>
#include <boost/xpressive/xpressive.hpp>

using namespace boost::xpressive;

std::map<std::string, std::string> replacements;

struct format1
{
    template<typename BidiIter>
    std::string operator()(match_results<BidiIter> const &what) const
    {
        return replacements[what[1].str()];
    }
};

struct format2
{
    template<typename BidiIter, typename OutIter>
    OutIter operator()(match_results<BidiIter> const &what, OutIter out) const
    {
        std::map<std::string, std::string>::const_iterator where = replacements.find(what[1].str());
        if(where != replacements.end())
            out = std::copy((*where).second.begin(), (*where).second.end(), out);
        return out;
    }
};

struct format3
{
    template<typename BidiIter, typename OutIter>
    OutIter operator()(match_results<BidiIter> const &what, OutIter out, regex_constants::match_flag_type) const
    {
        std::map<std::string, std::string>::const_iterator where = replacements.find(what[1].str());
        if(where != replacements.end())
            out = std::copy((*where).second.begin(), (*where).second.end(), out);
        return out;
    }
};

std::string format_fun(smatch const &what)
{
    return replacements[what[1].str()];
}

std::string c_format_fun(cmatch const &what)
{
    return replacements[what[1].str()];
}

void test_main()
{
    replacements["X"] = "this";
    replacements["Y"] = "that";

    std::string input("\"$(X)\" has the value \"$(Y)\""), output;
    sregex rx = sregex::compile("\\$\\(([^\\)]+)\\)");
    cregex crx = cregex::compile("\\$\\(([^\\)]+)\\)");

    std::string result("\"this\" has the value \"that\"");

    output = regex_replace(input, rx, format1());
    BOOST_CHECK_EQUAL(output, result);

    output = regex_replace(input.c_str(), crx, format1());
    BOOST_CHECK_EQUAL(output, result);

    output = regex_replace(input, rx, format2());
    BOOST_CHECK_EQUAL(output, result);

    output = regex_replace(input.c_str(), crx, format2());
    BOOST_CHECK_EQUAL(output, result);

    output = regex_replace(input, rx, format3());
    BOOST_CHECK_EQUAL(output, result);

    output = regex_replace(input.c_str(), crx, format3());
    BOOST_CHECK_EQUAL(output, result);

    output = regex_replace(input, rx, format_fun);
    BOOST_CHECK_EQUAL(output, result);

    output = regex_replace(input.c_str(), crx, c_format_fun);
    BOOST_CHECK_EQUAL(output, result);

    output = regex_replace(input, rx, &format_fun);
    BOOST_CHECK_EQUAL(output, result);

    output = regex_replace(input.c_str(), crx, &c_format_fun);
    BOOST_CHECK_EQUAL(output, result);

    output.clear();
    regex_replace(std::back_inserter(output), input.begin(), input.end(), rx, format1());
    BOOST_CHECK_EQUAL(output, result);

    output.clear();
    regex_replace(std::back_inserter(output), input.begin(), input.end(), rx, format2());
    BOOST_CHECK_EQUAL(output, result);

    output.clear();
    regex_replace(std::back_inserter(output), input.begin(), input.end(), rx, format2());
    BOOST_CHECK_EQUAL(output, result);

    output.clear();
    regex_replace(std::back_inserter(output), input.begin(), input.end(), rx, format_fun);
    BOOST_CHECK_EQUAL(output, result);

    output.clear();
    regex_replace(std::back_inserter(output), input.begin(), input.end(), rx, &format_fun);
    BOOST_CHECK_EQUAL(output, result);
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test_format");
    test->add(BOOST_TEST_CASE(&test_main));
    return test;
}
