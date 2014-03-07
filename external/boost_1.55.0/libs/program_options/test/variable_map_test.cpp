// Copyright Vladimir Prus 2002-2004.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/detail/utf8_codecvt_facet.hpp>
using namespace boost::program_options;
// We'll use po::value everywhere to workaround vc6 bug.
namespace po = boost::program_options;

#include <boost/function.hpp>
using namespace boost;

#include <sstream>
using namespace std;

#include "minitest.hpp"

vector<string> sv(const char* array[], unsigned size)
{
    vector<string> r;
    for (unsigned i = 0; i < size; ++i)
        r.push_back(array[i]);
    return r;
}

void test_variable_map()
{
    options_description desc;
    desc.add_options()
        ("foo,f", new untyped_value)
        ("bar,b", po::value<string>())
        ("biz,z", po::value<string>())
        ("baz", new untyped_value())
        ("output,o", new untyped_value(), "")
        ;
    const char* cmdline3_[] = { "--foo='12'", "--bar=11", "-z3", "-ofoo" };
    vector<string> cmdline3 = sv(cmdline3_,
                                 sizeof(cmdline3_)/sizeof(const char*));
    parsed_options a3 = command_line_parser(cmdline3).options(desc).run();
    variables_map vm;
    store(a3, vm);
    notify(vm);
    BOOST_REQUIRE(vm.size() == 4);
    BOOST_CHECK(vm["foo"].as<string>() == "'12'");
    BOOST_CHECK(vm["bar"].as<string>() == "11");
    BOOST_CHECK(vm.count("biz") == 1);
    BOOST_CHECK(vm["biz"].as<string>() == "3");
    BOOST_CHECK(vm["output"].as<string>() == "foo");

    int i;
    desc.add_options()
    ("zee", bool_switch(), "")
    ("zak", po::value<int>(&i), "")
    ("opt", bool_switch(), "");

    const char* cmdline4_[] = { "--zee", "--zak=13" };
    vector<string> cmdline4 = sv(cmdline4_,
                                 sizeof(cmdline4_)/sizeof(const char*));
    parsed_options a4 = command_line_parser(cmdline4).options(desc).run();

    variables_map vm2;
    store(a4, vm2);
    notify(vm2);
    BOOST_REQUIRE(vm2.size() == 3);
    BOOST_CHECK(vm2["zee"].as<bool>() == true);
    BOOST_CHECK(vm2["zak"].as<int>() == 13);
    BOOST_CHECK(vm2["opt"].as<bool>() == false);
    BOOST_CHECK(i == 13);

    options_description desc2;
    desc2.add_options()
    ("vee", po::value<string>()->default_value("42"))
    ("voo", po::value<string>())
    ("iii", po::value<int>()->default_value(123))
    ;
    const char* cmdline5_[] = { "--voo=1" };
    vector<string> cmdline5 = sv(cmdline5_,
                                 sizeof(cmdline5_)/sizeof(const char*));
    parsed_options a5 = command_line_parser(cmdline5).options(desc2).run();

    variables_map vm3;
    store(a5, vm3);
    notify(vm3);
    BOOST_REQUIRE(vm3.size() == 3);
    BOOST_CHECK(vm3["vee"].as<string>() == "42");
    BOOST_CHECK(vm3["voo"].as<string>() == "1");
    BOOST_CHECK(vm3["iii"].as<int>() == 123);

    options_description desc3;
    desc3.add_options()
    ("imp", po::value<int>()->implicit_value(100))
    ("iim", po::value<int>()->implicit_value(200)->default_value(201))
    ("mmp,m", po::value<int>()->implicit_value(123)->default_value(124))
    ("foo", po::value<int>())
    ;
    /* The -m option is implicit. It does not have value in inside the token,
       and we should not grab the next token.  */
    const char* cmdline6_[] = {  "--imp=1", "-m", "--foo=1" };
    vector<string> cmdline6 = sv(cmdline6_,
                                 sizeof(cmdline6_)/sizeof(const char*));
    parsed_options a6 = command_line_parser(cmdline6).options(desc3).run();

    variables_map vm4;
    store(a6, vm4);
    notify(vm4);
    BOOST_REQUIRE(vm4.size() == 4);
    BOOST_CHECK(vm4["imp"].as<int>() == 1);
    BOOST_CHECK(vm4["iim"].as<int>() == 201);
    BOOST_CHECK(vm4["mmp"].as<int>() == 123);
}

int stored_value;
void notifier(const vector<int>& v)
{
    stored_value = v.front();
}

void test_semantic_values()
{
    options_description desc;
    desc.add_options()
    ("foo", new untyped_value())
    ("bar", po::value<int>())
    ("biz", po::value< vector<string> >())
    ("baz", po::value< vector<string> >()->multitoken())
    ("int", po::value< vector<int> >()->notifier(&notifier))
    ;

       
    parsed_options parsed(&desc);
    vector<option>& options = parsed.options;
    vector<string> v;
    v.push_back("q");
    options.push_back(option("foo", vector<string>(1, "1")));
    options.push_back(option("biz", vector<string>(1, "a")));
    options.push_back(option("baz", v));
    options.push_back(option("bar", vector<string>(1, "1")));
    options.push_back(option("biz", vector<string>(1, "b x")));
    v.push_back("w");
    options.push_back(option("baz", v));

    variables_map vm;
    store(parsed, vm);
    notify(vm);
    BOOST_REQUIRE(vm.count("biz") == 1);
    BOOST_REQUIRE(vm.count("baz") == 1);
    const vector<string> av = vm["biz"].as< vector<string> >();
    const vector<string> av2 = vm["baz"].as< vector<string> >();
    string exp1[] = { "a", "b x" };
    BOOST_CHECK(av == vector<string>(exp1, exp1 + 2));
    string exp2[] = { "q", "q", "w" };
    BOOST_CHECK(av2 == vector<string>(exp2, exp2 + 3));

    options.push_back(option("int", vector<string>(1, "13")));

    variables_map vm2;
    store(parsed, vm2);
    notify(vm2);
    BOOST_REQUIRE(vm2.count("int") == 1);
    BOOST_CHECK(vm2["int"].as< vector<int> >() == vector<int>(1, 13));
    BOOST_CHECK_EQUAL(stored_value, 13);

    vector<option> saved_options = options;
    
    options.push_back(option("bar", vector<string>(1, "2")));
    variables_map vm3;
    BOOST_CHECK_THROW(store(parsed, vm3), multiple_occurrences);

    options = saved_options;
    // Now try passing two int in one 'argv' element.
    // This should not work.
    options.push_back(option("int", vector<string>(1, "2 3")));
    variables_map vm4;
    BOOST_CHECK_THROW(store(parsed, vm4), validation_error);
}

void test_priority()
{
    options_description desc;
    desc.add_options()
    // Value of this option will be specified in two sources,
    // and only first one should be used.
    ("first", po::value< vector<int > >())
    // Value of this option will have default value in the first source,
    // and explicit assignment in the second, so the second should be used.
    ("second", po::value< vector<int > >()->default_value(vector<int>(1, 1), ""))
    ("aux", po::value< vector<int > >())
     // This will have values in both sources, and values should be combined
    ("include", po::value< vector<int> >()->composing())
    ;

    const char* cmdline1_[] = { "--first=1", "--aux=10", "--first=3", "--include=1" };
    vector<string> cmdline1 = sv(cmdline1_, 
                                 sizeof(cmdline1_)/sizeof(const char*));

    parsed_options p1 = command_line_parser(cmdline1).options(desc).run();

    const char* cmdline2_[] = { "--first=12", "--second=7", "--include=7" };
    vector<string> cmdline2 = sv(cmdline2_, 
                                 sizeof(cmdline2_)/sizeof(const char*));

    parsed_options p2 = command_line_parser(cmdline2).options(desc).run();

    variables_map vm;
    store(p1, vm);

    BOOST_REQUIRE(vm.count("first") == 1);    
    BOOST_REQUIRE(vm["first"].as< vector<int> >().size() == 2);
    BOOST_CHECK_EQUAL(vm["first"].as< vector<int> >()[0], 1);
    BOOST_CHECK_EQUAL(vm["first"].as< vector<int> >()[1], 3);

    BOOST_REQUIRE(vm.count("second") == 1);    
    BOOST_REQUIRE(vm["second"].as< vector<int> >().size() == 1);
    BOOST_CHECK_EQUAL(vm["second"].as< vector<int> >()[0], 1);

    store(p2, vm);

    // Value should not change.
    BOOST_REQUIRE(vm.count("first") == 1);    
    BOOST_REQUIRE(vm["first"].as< vector<int> >().size() == 2);
    BOOST_CHECK_EQUAL(vm["first"].as< vector<int> >()[0], 1);
    BOOST_CHECK_EQUAL(vm["first"].as< vector<int> >()[1], 3);

    // Value should change to 7
    BOOST_REQUIRE(vm.count("second") == 1);    
    BOOST_REQUIRE(vm["second"].as< vector<int> >().size() == 1);
    BOOST_CHECK_EQUAL(vm["second"].as< vector<int> >()[0], 7);

    BOOST_REQUIRE(vm.count("include") == 1);
    BOOST_REQUIRE(vm["include"].as< vector<int> >().size() == 2);
    BOOST_CHECK_EQUAL(vm["include"].as< vector<int> >()[0], 1);
    BOOST_CHECK_EQUAL(vm["include"].as< vector<int> >()[1], 7);
}

void test_multiple_assignments_with_different_option_description()
{
    // Test that if we store option twice into the same variable_map,
    // and some of the options stored the first time are not present
    // in the options descrription provided the second time, we don't crash.

    options_description desc1("");    
    desc1.add_options()
        ("help,h", "")
        ("includes", po::value< vector<string> >()->composing(), "");
        ;

    options_description desc2("");
    desc2.add_options()
        ("output,o", "");

    vector<string> input1;
    input1.push_back("--help");
    input1.push_back("--includes=a");
    parsed_options p1 = command_line_parser(input1).options(desc1).run();

    vector<string> input2;
    input1.push_back("--output");
    parsed_options p2 = command_line_parser(input2).options(desc2).run();

    vector<string> input3;
    input3.push_back("--includes=b");
    parsed_options p3 = command_line_parser(input3).options(desc1).run();


    variables_map vm;
    store(p1, vm);
    store(p2, vm);
    store(p3, vm);

    BOOST_REQUIRE(vm.count("help") == 1);
    BOOST_REQUIRE(vm.count("includes") == 1);
    BOOST_CHECK_EQUAL(vm["includes"].as< vector<string> >()[0], "a");
    BOOST_CHECK_EQUAL(vm["includes"].as< vector<string> >()[1], "b");

} 

int main(int, char* [])
{
    test_variable_map();
    test_semantic_values();
    test_priority();
    test_multiple_assignments_with_different_option_description();
    return 0;
}

