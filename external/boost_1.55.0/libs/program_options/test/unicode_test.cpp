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

// Test that unicode input is forwarded to unicode option without
// problems.
void test_unicode_to_unicode()
{
    options_description desc;

    desc.add_options()
        ("foo", po::wvalue<wstring>(), "unicode option")
        ;

    vector<wstring> args;
    args.push_back(L"--foo=\x044F");

    variables_map vm;
    basic_parsed_options<wchar_t> parsed = 
        wcommand_line_parser(args).options(desc).run();
    store(parsed, vm);

    BOOST_CHECK(vm["foo"].as<wstring>() == L"\x044F");
    BOOST_CHECK(parsed.options[0].original_tokens.size() == 1);
    BOOST_CHECK(parsed.options[0].original_tokens[0] == L"--foo=\x044F");
}

// Test that unicode input is property converted into
// local 8 bit string. To test this, make local 8 bit encoding
// be utf8.
void test_unicode_to_native()
{
    std::codecvt<wchar_t, char, mbstate_t>* facet = 
        new boost::program_options::detail::utf8_codecvt_facet;
    locale::global(locale(locale(), facet));

    options_description desc;

    desc.add_options()
        ("foo", po::value<string>(), "unicode option")
        ;

    vector<wstring> args;
    args.push_back(L"--foo=\x044F");

    variables_map vm;
    store(wcommand_line_parser(args).options(desc).run(), vm);

    BOOST_CHECK(vm["foo"].as<string>() == "\xD1\x8F");    
}

void test_native_to_unicode()
{
    std::codecvt<wchar_t, char, mbstate_t>* facet = 
        new boost::program_options::detail::utf8_codecvt_facet;
    locale::global(locale(locale(), facet));

    options_description desc;

    desc.add_options()
        ("foo", po::wvalue<wstring>(), "unicode option")
        ;

    vector<string> args;
    args.push_back("--foo=\xD1\x8F");

    variables_map vm;
    store(command_line_parser(args).options(desc).run(), vm);

    BOOST_CHECK(vm["foo"].as<wstring>() == L"\x044F");    
}

vector<wstring> sv(const wchar_t* array[], unsigned size)
{
    vector<wstring> r;
    for (unsigned i = 0; i < size; ++i)
        r.push_back(array[i]);
    return r;
}

void check_value(const woption& option, const char* name, const wchar_t* value)
{
    BOOST_CHECK(option.string_key == name);
    BOOST_REQUIRE(option.value.size() == 1);
    BOOST_CHECK(option.value.front() == value);
}

void test_command_line()
{
    options_description desc;
    desc.add_options()
        ("foo,f", new untyped_value(), "")
        // Explicit qualification is a workaround for vc6
        ("bar,b", po::value<std::string>(), "")
        ("baz", new untyped_value())
        ("plug*", new untyped_value())
        ;

    const wchar_t* cmdline4_[] = { L"--foo=1\u0FF52", L"-f4", L"--bar=11",
                             L"-b4", L"--plug3=10"};
    vector<wstring> cmdline4 = sv(cmdline4_,
                                  sizeof(cmdline4_)/sizeof(cmdline4_[0]));
    vector<woption> a4 = 
        wcommand_line_parser(cmdline4).options(desc).run().options;

    BOOST_REQUIRE(a4.size() == 5);

    check_value(a4[0], "foo", L"1\u0FF52");
    check_value(a4[1], "foo", L"4");
    check_value(a4[2], "bar", L"11");
}

// Since we've already tested conversion between parser encoding and
// option encoding, all we need to check for config file is that
// when reading wistream, it generates proper UTF8 data.
void test_config_file()
{
    std::codecvt<wchar_t, char, mbstate_t>* facet = 
        new boost::program_options::detail::utf8_codecvt_facet;
    locale::global(locale(locale(), facet));

    options_description desc;

    desc.add_options()
        ("foo", po::value<string>(), "unicode option")
        ;

    std::wstringstream stream(L"foo = \x044F");

    variables_map vm;
    store(parse_config_file(stream, desc), vm);

    BOOST_CHECK(vm["foo"].as<string>() == "\xD1\x8F");    
}

int main(int, char* [])
{
    test_unicode_to_unicode();
    test_unicode_to_native();
    test_native_to_unicode();
    test_command_line();
    test_config_file();
    return 0;
}

