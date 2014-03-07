//  Boost string_algo library find_format_test.cpp file  ------------------//

//  Copyright (c) 2009 Steven Watanabe
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/algorithm/string/find_format.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/formatter.hpp>

// Include unit test framework
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <boost/test/test_tools.hpp>

// We're only using const_formatter.
template<class Formatter>
struct formatter_result {
    typedef boost::iterator_range<const char*> type;
};

template<class Formatter>
struct checked_formatter {
public:
    checked_formatter(const Formatter& formatter) : formatter_(formatter) {}
    template< typename T >
    typename formatter_result<Formatter>::type operator()( const T & s ) const {
        BOOST_CHECK( !s.empty() );
        return formatter_(s);
    }
private:
    Formatter formatter_;
};

template<class Formatter>
checked_formatter<Formatter>
make_checked_formatter(const Formatter& formatter) {
    return checked_formatter<Formatter>(formatter);
}

void find_format_test()
{
    const std::string source = "$replace $replace";
    std::string expected = "ok $replace";
    std::string output(80, '\0');

    std::string::iterator pos =
        boost::find_format_copy(
            output.begin(),
            source,
            boost::first_finder("$replace"),
            make_checked_formatter(boost::const_formatter("ok")));
    BOOST_CHECK(pos == output.begin() + expected.size());
    output.erase(std::remove(output.begin(), output.end(), '\0'), output.end());
    BOOST_CHECK_EQUAL(output, expected);

    output =
        boost::find_format_copy(
            source,
            boost::first_finder("$replace"),
            make_checked_formatter(boost::const_formatter("ok")));
    BOOST_CHECK_EQUAL(output, expected);

    // now try finding a string that doesn't exist
    output.resize(80);
    pos =
        boost::find_format_copy(
            output.begin(),
            source,
            boost::first_finder("$noreplace"),
            make_checked_formatter(boost::const_formatter("bad")));
    BOOST_CHECK(pos == output.begin() + source.size());
    output.erase(std::remove(output.begin(), output.end(), '\0'), output.end());
    BOOST_CHECK_EQUAL(output, source);

    output =
        boost::find_format_copy(
            source,
            boost::first_finder("$noreplace"),
            make_checked_formatter(boost::const_formatter("bad")));
    BOOST_CHECK_EQUAL(output, source);

    // in place version
    output = source;
    boost::find_format(
        output,
        boost::first_finder("$replace"),
        make_checked_formatter(boost::const_formatter("ok")));
    BOOST_CHECK_EQUAL(output, expected);
    output = source;
    boost::find_format(
        output,
        boost::first_finder("$noreplace"),
        make_checked_formatter(boost::const_formatter("bad")));
    BOOST_CHECK_EQUAL(output, source);
}

void find_format_all_test()
{
    const std::string source = "$replace $replace";
    std::string expected = "ok ok";
    std::string output(80, '\0');

    std::string::iterator pos =
        boost::find_format_all_copy(output.begin(),
                                source,
                                boost::first_finder("$replace"),
                                boost::const_formatter("ok"));
    BOOST_CHECK(pos == output.begin() + expected.size());
    output.erase(std::remove(output.begin(), output.end(), '\0'), output.end());
    BOOST_CHECK_EQUAL(output, expected);

    output =
        boost::find_format_all_copy(
            source,
            boost::first_finder("$replace"),
            make_checked_formatter(boost::const_formatter("ok")));
    BOOST_CHECK_EQUAL(output, expected);

    // now try finding a string that doesn't exist
    output.resize(80);
    pos =
        boost::find_format_all_copy(
            output.begin(),
            source,
            boost::first_finder("$noreplace"),
            make_checked_formatter(boost::const_formatter("bad")));
    BOOST_CHECK(pos == output.begin() + source.size());
    output.erase(std::remove(output.begin(), output.end(), '\0'), output.end());
    BOOST_CHECK_EQUAL(output, source);

    output =
        boost::find_format_all_copy(
            source,
            boost::first_finder("$noreplace"),
            make_checked_formatter(boost::const_formatter("bad")));
    BOOST_CHECK_EQUAL(output, source);

    // in place version
    output = source;
    boost::find_format_all(
        output,
        boost::first_finder("$replace"),
        make_checked_formatter(boost::const_formatter("ok")));
    BOOST_CHECK_EQUAL(output, expected);
    output = source;
    boost::find_format_all(
        output,
        boost::first_finder("$noreplace"),
        make_checked_formatter(boost::const_formatter("bad")));
    BOOST_CHECK_EQUAL(output, source);
}

BOOST_AUTO_TEST_CASE( test_main )
{
    find_format_test();
    find_format_all_test();
}
