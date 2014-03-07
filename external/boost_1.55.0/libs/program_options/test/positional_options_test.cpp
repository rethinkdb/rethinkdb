// Copyright Vladimir Prus 2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/program_options/positional_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
using namespace boost::program_options;
// We'll use po::value everywhere to workaround vc6 bug.
namespace po = boost::program_options;


#include <boost/limits.hpp>

#include "minitest.hpp"

#include <vector>
using namespace std;

void test_positional_options()
{
    positional_options_description p;
    p.add("first", 1);

    BOOST_CHECK_EQUAL(p.max_total_count(), 1u);
    BOOST_CHECK_EQUAL(p.name_for_position(0), "first");

    p.add("second", 2);

    BOOST_CHECK_EQUAL(p.max_total_count(), 3u);
    BOOST_CHECK_EQUAL(p.name_for_position(0), "first");
    BOOST_CHECK_EQUAL(p.name_for_position(1), "second");
    BOOST_CHECK_EQUAL(p.name_for_position(2), "second");

    p.add("third", -1);

    BOOST_CHECK_EQUAL(p.max_total_count(), (std::numeric_limits<unsigned>::max)());
    BOOST_CHECK_EQUAL(p.name_for_position(0), "first");
    BOOST_CHECK_EQUAL(p.name_for_position(1), "second");
    BOOST_CHECK_EQUAL(p.name_for_position(2), "second");
    BOOST_CHECK_EQUAL(p.name_for_position(3), "third");
    BOOST_CHECK_EQUAL(p.name_for_position(10000), "third");
}

void test_parsing()
{
    options_description desc;
    desc.add_options()
        ("first", po::value<int>())
        ("second", po::value<int>())
        ("input-file", po::value< vector<string> >())
        ("some-other", po::value<string>())
    ;

    positional_options_description p;
    p.add("input-file", 2).add("some-other", 1);

    vector<string> args;
    args.push_back("--first=10");
    args.push_back("file1");
    args.push_back("--second=10");
    args.push_back("file2");
    args.push_back("file3");

    // Check that positional options are handled.
    parsed_options parsed = 
        command_line_parser(args).options(desc).positional(p).run();

    BOOST_REQUIRE(parsed.options.size() == 5);
    BOOST_CHECK_EQUAL(parsed.options[1].string_key, "input-file");
    BOOST_CHECK_EQUAL(parsed.options[1].value[0], "file1");
    BOOST_CHECK_EQUAL(parsed.options[3].string_key, "input-file");
    BOOST_CHECK_EQUAL(parsed.options[3].value[0], "file2");
    BOOST_CHECK_EQUAL(parsed.options[4].value[0], "file3");

    args.push_back("file4");

    // Check that excessive number of positional options is detected.
    BOOST_CHECK_THROW(command_line_parser(args).options(desc).positional(p)
                      .run(),
                      too_many_positional_options_error);
}

int main(int, char* [])
{
    test_positional_options();
    test_parsing();
    return 0;
}

