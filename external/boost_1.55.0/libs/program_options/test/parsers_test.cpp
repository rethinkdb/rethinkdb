// Copyright Vladimir Prus 2002-2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
using namespace boost::program_options;
// We'll use po::value everywhere to workaround vc6 bug.
namespace po = boost::program_options;

#include <boost/function.hpp>
using namespace boost;

#include <sstream>
#include <iostream>
using namespace std;

#if defined(__sun)
#include <stdlib.h> // for putenv on solaris
#else
#include <cstdlib> // for putenv
#endif

#include "minitest.hpp"

#define TEST_CHECK_THROW(expression, exception, description) \
    try \
    { \
        expression; \
        BOOST_ERROR(description);\
        throw 10; \
    } \
    catch(exception &) \
    { \
    }

pair<string, vector< vector<string> > > msp(const string& s1)
{
    return std::make_pair(s1, vector< vector<string> >());
}


pair<string, vector< vector<string> > > msp(const string& s1, const string& s2)
{
    vector< vector<string> > v(1);
    v[0].push_back(s2);
    return std::make_pair(s1, v);
}

void check_value(const option& option, const char* name, const char* value)
{
    BOOST_CHECK(option.string_key == name);
    BOOST_REQUIRE(option.value.size() == 1);
    BOOST_CHECK(option.value.front() == value);
}

vector<string> sv(const char* array[], unsigned size)
{
    vector<string> r;
    for (unsigned i = 0; i < size; ++i)
        r.push_back(array[i]);
    return r;
}

pair<string, string> additional_parser(const std::string&)
{
    return pair<string, string>();
}

void test_command_line()
{
    // The following commented out blocks used to test parsing
    // command line without syntax specification behaviour.
    // It is disabled now and probably will never be enabled again:
    // it is not possible to figure out what command line means without
    // user's help.
    #if 0
    char* cmdline1[] = { "--a", "--b=12", "-f", "-g4", "-", "file" };

    options_and_arguments a1 =
        parse_command_line(cmdline1,
                           cmdline1 + sizeof(cmdline1)/sizeof(cmdline1[0]));

    BOOST_REQUIRE(a1.options().size() == 4);
    BOOST_CHECK(a1.options()[0] == msp("a", ""));
    BOOST_CHECK(a1.options()[1] == msp("b", "12"));
    BOOST_CHECK(a1.options()[2] == msp("-f", ""));
    BOOST_CHECK(a1.options()[3] == msp("-g", "4"));
    BOOST_REQUIRE(a1.arguments().size() == 2);
    BOOST_CHECK(a1.arguments()[0] == "-");
    BOOST_CHECK(a1.arguments()[1] == "file");

    char* cmdline2[] = { "--a", "--", "file" };

    options_and_arguments a2 =
        parse_command_line(cmdline2,
                           cmdline2 + sizeof(cmdline2)/sizeof(cmdline2[0]));

    BOOST_REQUIRE(a2.options().size() == 1);
    BOOST_CHECK(a2.options()[0] == msp("a", ""));
    BOOST_CHECK(a2.arguments().size() == 1);
    BOOST_CHECK(a2.arguments()[0] == "file");
    #endif
    
    options_description desc;
    desc.add_options()
        ("foo,f", new untyped_value(), "")
        // Explicit qualification is a workaround for vc6
        ("bar,b", po::value<std::string>(), "")
        ("baz", new untyped_value())
        ("plug*", new untyped_value())
        ;
    const char* cmdline3_[] = { "--foo=12", "-f4", "--bar=11", "-b4",
                          "--plug3=10"};
    vector<string> cmdline3 = sv(cmdline3_,
                                 sizeof(cmdline3_)/sizeof(const char*));
    vector<option> a3 = 
        command_line_parser(cmdline3).options(desc).run().options;
                       
    BOOST_CHECK_EQUAL(a3.size(), 5u);

    check_value(a3[0], "foo", "12");
    check_value(a3[1], "foo", "4");
    check_value(a3[2], "bar", "11");
    check_value(a3[3], "bar", "4");
    check_value(a3[4], "plug3", "10");

    // Regression test: check that '0' as style is interpreted as 
    // 'default_style'
    vector<option> a4 = 
        parse_command_line(sizeof(cmdline3_)/sizeof(const char*), cmdline3_, 
                                                               desc, 0, additional_parser).options;

    BOOST_CHECK_EQUAL(a4.size(), 4u);
    check_value(a4[0], "foo", "4");
    check_value(a4[1], "bar", "11");

    // Check that we don't crash on empty values of type 'string'
    const char* cmdline4[] = {"", "--open", ""};
    options_description desc2;
    desc2.add_options()
        ("open", po::value<string>())
        ;
    variables_map vm;
    po::store(po::parse_command_line(sizeof(cmdline4)/sizeof(const char*), const_cast<char**>(cmdline4), desc2), vm);

    const char* cmdline5[] = {"", "-p7", "-o", "1", "2", "3", "-x8"};
    options_description desc3;
    desc3.add_options()
        (",p", po::value<string>())
        (",o", po::value<string>()->multitoken())
        (",x", po::value<string>())
        ;
    vector<option> a5 = 
        parse_command_line(sizeof(cmdline5)/sizeof(const char*), const_cast<char**>(cmdline5), 
                                                                     desc3, 0, additional_parser).options;
    BOOST_CHECK_EQUAL(a5.size(), 3u);
    check_value(a5[0], "-p", "7");
    BOOST_REQUIRE(a5[1].value.size() == 3);
    BOOST_CHECK_EQUAL(a5[1].string_key, "-o");
    BOOST_CHECK_EQUAL(a5[1].value[0], "1");
    BOOST_CHECK_EQUAL(a5[1].value[1], "2");
    BOOST_CHECK_EQUAL(a5[1].value[2], "3");
    check_value(a5[2], "-x", "8");   


    po::options_description desc4( "" );
    desc4.add_options()
        ( "multitoken,m",
          po::value< std::vector< std::string > >()->multitoken(),
          "values"
            )
        ( "file",
          po::value< std::string >(),
          "the file to process"
            )
        ;

    po::positional_options_description p;
    p.add( "file", 1 );

    const char* cmdline6[] = {"", "-m", "token1", "token2", "--", "some_file"};
    vector<option> a6 = 
        command_line_parser(sizeof(cmdline6)/sizeof(const char*), const_cast<char**>(cmdline6)).options(desc4).positional(p)
        .run().options;
    BOOST_CHECK_EQUAL(a6.size(), 2u);
    BOOST_REQUIRE(a6[0].value.size() == 2);
    BOOST_CHECK_EQUAL(a6[0].string_key, "multitoken");
    BOOST_CHECK_EQUAL(a6[0].value[0], "token1");
    BOOST_CHECK_EQUAL(a6[0].value[1], "token2");
    BOOST_CHECK_EQUAL(a6[1].string_key, "file");
    BOOST_REQUIRE(a6[1].value.size() == 1);
    BOOST_CHECK_EQUAL(a6[1].value[0], "some_file");
}

void test_config_file(const char* config_file)
{
    options_description desc;
    desc.add_options()
        ("gv1", new untyped_value)
        ("gv2", new untyped_value)
        ("empty_value", new untyped_value)
        ("plug*", new untyped_value)
        ("m1.v1", new untyped_value)
        ("m1.v2", new untyped_value)
        ("b", bool_switch())
    ;

    const char content1[] =
    " gv1 = 0#asd\n"
    "empty_value = \n"
    "plug3 = 7\n"
    "b = true\n"
    "[m1]\n"
    "v1 = 1\n"
    "\n"
    "v2 = 2\n"    
    ;

    stringstream ss(content1);
    vector<option> a1 = parse_config_file(ss, desc).options;
    BOOST_REQUIRE(a1.size() == 6);
    check_value(a1[0], "gv1", "0");
    check_value(a1[1], "empty_value", "");
    check_value(a1[2], "plug3", "7");
    check_value(a1[3], "b", "true");
    check_value(a1[4], "m1.v1", "1");
    check_value(a1[5], "m1.v2", "2");
    
    // same test, but now options come from file 
    vector<option> a2 = parse_config_file<char>(config_file, desc).options;
    BOOST_REQUIRE(a2.size() == 6);
    check_value(a2[0], "gv1", "0");
    check_value(a2[1], "empty_value", "");
    check_value(a2[2], "plug3", "7");
    check_value(a2[3], "b", "true");
    check_value(a2[4], "m1.v1", "1");
    check_value(a2[5], "m1.v2", "2");
}

void test_environment()
{
    options_description desc;
    desc.add_options()
        ("foo", new untyped_value, "")
        ("bar", new untyped_value, "")
        ;

#if defined(_WIN32) && ! defined(__BORLANDC__)
    _putenv("PO_TEST_FOO=1");
#else
    putenv(const_cast<char*>("PO_TEST_FOO=1"));
#endif
    parsed_options p = parse_environment(desc, "PO_TEST_");

    BOOST_REQUIRE(p.options.size() == 1);
    BOOST_CHECK(p.options[0].string_key == "foo");
    BOOST_REQUIRE(p.options[0].value.size() == 1);
    BOOST_CHECK(p.options[0].value[0] == "1");

    //TODO: since 'bar' does not allow a value, it cannot appear in environemt,
    // which already has a value.
}

void test_unregistered()
{
    options_description desc;

    const char* cmdline1_[] = { "--foo=12", "--bar", "1"};
    vector<string> cmdline1 = sv(cmdline1_,
                                 sizeof(cmdline1_)/sizeof(const char*));
    vector<option> a1 = 
        command_line_parser(cmdline1).options(desc).allow_unregistered().run()
        .options;

    BOOST_REQUIRE(a1.size() == 3);
    BOOST_CHECK(a1[0].string_key == "foo");
    BOOST_CHECK(a1[0].unregistered == true);
    BOOST_REQUIRE(a1[0].value.size() == 1);
    BOOST_CHECK(a1[0].value[0] == "12");
    BOOST_CHECK(a1[1].string_key == "bar");
    BOOST_CHECK(a1[1].unregistered == true);
    BOOST_CHECK(a1[2].string_key == "");
    BOOST_CHECK(a1[2].unregistered == false);
    

    vector<string> a2 = collect_unrecognized(a1, include_positional);
    BOOST_CHECK(a2[0] == "--foo=12");
    BOOST_CHECK(a2[1] == "--bar");
    BOOST_CHECK(a2[2] == "1");

    // Test that storing unregisted options has no effect
    variables_map vm;
    
    store(command_line_parser(cmdline1).options(desc).
          allow_unregistered().run(),
          vm);

    BOOST_CHECK_EQUAL(vm.size(), 0u);   


    const char content1[] =
    "gv1 = 0\n"
    "[m1]\n"
    "v1 = 1\n"
    ;

    stringstream ss(content1);
    vector<option> a3 = parse_config_file(ss, desc, true).options;
    BOOST_REQUIRE(a3.size() == 2);
    cout << "XXX" << a3[0].value.front() << "\n";
    check_value(a3[0], "gv1", "0");
    check_value(a3[1], "m1.v1", "1");
}

int main(int, char* av[])
{
    test_command_line();
    test_config_file(av[1]);
    test_environment();
    test_unregistered();
    return 0;
}

