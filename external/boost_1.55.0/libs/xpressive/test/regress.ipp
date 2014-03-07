//////////////////////////////////////////////////////////////////////////////
// regress.ipp
//
//  (C) Copyright Eric Niebler 2004.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
 Revision history:
   7 March 2004 : Initial version.
*/

#include <locale>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/test/unit_test.hpp>

#if defined(_MSC_VER) && defined(_DEBUG)
# define _CRTDBG_MAP_ALLOC
# include <crtdbg.h>
#endif

#if defined(BOOST_XPRESSIVE_TEST_WREGEX) && !defined(BOOST_XPRESSIVE_NO_WREGEX)
namespace std
{
    inline std::ostream &operator <<(std::ostream &sout, std::wstring const &wstr)
    {
        for(std::size_t n = 0; n < wstr.size(); ++n)
            sout.put(BOOST_USE_FACET(std::ctype<wchar_t>, std::locale()).narrow(wstr[n], '?'));
        return sout;
    }
}
#endif

#define BOOST_XPR_CHECK(pred)                                                   \
    if(pred) {} else { BOOST_ERROR(case_ << #pred); }

using namespace boost::unit_test;
using namespace boost::xpressive;

//////////////////////////////////////////////////////////////////////////////
// xpr_test_case
template<typename Char>
struct xpr_test_case
{
    typedef std::basic_string<Char> string_type;
    std::string section;
    string_type str;
    string_type pat;
    string_type sub;
    string_type res;
    regex_constants::syntax_option_type syntax_flags;
    regex_constants::match_flag_type match_flags;
    std::vector<string_type> br;

    xpr_test_case()
    {
        this->reset();
    }

    void reset()
    {
        this->section.clear();
        this->str.clear();
        this->pat.clear();
        this->sub.clear();
        this->res.clear();
        this->br.clear();
        this->syntax_flags = regex_constants::ECMAScript;
        this->match_flags = regex_constants::match_default | regex_constants::format_first_only;
    }
};

//////////////////////////////////////////////////////////////////////////////
// globals
std::ifstream in;
unsigned int test_count = 0;

// The global object that contains the current test case
xpr_test_case<char> test;

struct test_case_formatter
{
    friend std::ostream &operator <<(std::ostream &sout, test_case_formatter)
    {
        sout << test.section << " /" << test.pat << "/ : ";
        return sout;
    }
};

test_case_formatter const case_ = {};

#if defined(BOOST_XPRESSIVE_TEST_WREGEX) && !defined(BOOST_XPRESSIVE_NO_WREGEX)
///////////////////////////////////////////////////////////////////////////////
// widen
//  make a std::wstring from a std::string by widening according to the
//  current ctype<char> facet
inline std::wstring widen(std::string const &str)
{
    std::ctype<char> const &ct = BOOST_USE_FACET(std::ctype<char>, std::locale());
    std::wstring res;
    for(size_t i=0; i<str.size(); ++i)
    {
        res += ct.widen(str[i]);
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////
// widen
//  widens an entire test case
xpr_test_case<wchar_t> widen(xpr_test_case<char> const &test)
{
    xpr_test_case<wchar_t> wtest;
    wtest.section = test.section;
    wtest.str = ::widen(test.str);
    wtest.pat = ::widen(test.pat);
    wtest.sub = ::widen(test.sub);
    wtest.res = ::widen(test.res);
    wtest.syntax_flags = test.syntax_flags;
    wtest.match_flags = test.match_flags;
    wtest.br.reserve(test.br.size());
    for(std::size_t i = 0; i < test.br.size(); ++i)
    {
        wtest.br.push_back(::widen(test.br[i]));
    }
    return wtest;
}
#endif // BOOST_XPRESSIVE_NO_WREGEX

std::string escape(std::string str)
{
    for(std::string::size_type pos = 0; std::string::npos != (pos = str.find('\\', pos)); ++pos)
    {
        if(pos + 1 == str.size())
            break;

        switch(str[pos + 1])
        {
        case '\\': str.replace(pos, 2, "\\"); break;
        case 'n': str.replace(pos, 2, "\n"); break;
        case 'r': str.replace(pos, 2, "\r"); break;
        }
    }
    return str;
}

///////////////////////////////////////////////////////////////////////////////
// get_test
//   read the next section out of the input file, and fill out
//   the global variables
bool get_test()
{
    test.reset();
    bool first = true;
    std::string line;
    smatch what;

    sregex const rx_sec = '[' >> (s1= +_) >> ']';
    sregex const rx_str = "str=" >> (s1= *_);
    sregex const rx_pat = "pat=" >> (s1= *_);
    sregex const rx_flg = "flg=" >> (s1= *_);
    sregex const rx_sub = "sub=" >> (s1= *_);
    sregex const rx_res = "res=" >> (s1= *_);
    sregex const rx_br = "br" >> (s1= +digit) >> '=' >> (s2= *_);

    while(in.good())
    {
        std::getline(in, line);

        if(!line.empty() && '\r' == line[line.size()-1])
        {
            line.erase(line.size()-1);
        }

        if(regex_match(line, what, rx_sec))
        {
            if(!first)
            {
                if(what[1] != "end")
                {
                    BOOST_FAIL(("invalid input : " + line).c_str());
                }
                break;
            }

            first = false;
            test.section = what[1].str();
        }
        else if(regex_match(line, what, rx_str))
        {
            test.str = escape(what[1].str());
        }
        else if(regex_match(line, what, rx_pat))
        {
            test.pat = what[1].str();
        }
        else if(regex_match(line, what, rx_sub))
        {
            test.sub = what[1].str();
        }
        else if(regex_match(line, what, rx_res))
        {
            test.res = escape(what[1].str());
        }
        else if(regex_match(line, what, rx_flg))
        {
            std::string flg = what[1].str();

            if(std::string::npos != flg.find('i'))
            {
                test.syntax_flags = test.syntax_flags | regex_constants::icase;
            }
            if(std::string::npos == flg.find('m'))
            {
                test.syntax_flags = test.syntax_flags | regex_constants::single_line;
            }
            if(std::string::npos == flg.find('s'))
            {
                test.syntax_flags = test.syntax_flags | regex_constants::not_dot_newline;
            }
            if(std::string::npos != flg.find('x'))
            {
                test.syntax_flags = test.syntax_flags | regex_constants::ignore_white_space;
            }
            if(std::string::npos != flg.find('g'))
            {
                test.match_flags = test.match_flags & ~regex_constants::format_first_only;
            }
            if(std::string::npos != flg.find('a'))
            {
                test.match_flags = test.match_flags | regex_constants::format_all;
            }
            if(std::string::npos != flg.find('p'))
            {
                test.match_flags = test.match_flags | regex_constants::format_perl;
            }
            if(std::string::npos != flg.find('d'))
            {
                test.match_flags = test.match_flags | regex_constants::format_sed;
            }
        }
        else if(regex_match(line, what, rx_br))
        {
            std::size_t nbr = boost::lexical_cast<std::size_t>(what[1].str());

            if(nbr >= test.br.size())
            {
                test.br.resize(nbr + 1);
            }

            test.br[nbr] = escape(what[2].str());
        }
        else if(!line.empty() && ';' != line[0])
        {
            BOOST_FAIL((std::string("invalid input : ") + line).c_str());
        }
    }

    return !first;
}

///////////////////////////////////////////////////////////////////////////////
// run_test_impl
//   run the test
template<typename Char>
void run_test_impl(xpr_test_case<Char> const &test)
{
    try
    {
        Char const empty[] = {0};
        typedef typename std::basic_string<Char>::const_iterator iterator;
        basic_regex<iterator> rx = basic_regex<iterator>::compile(test.pat, test.syntax_flags);

        // Build the same regex for use with C strings
        basic_regex<Char const *> c_rx = basic_regex<Char const *>::compile(test.pat, test.syntax_flags);

        if(!test.res.empty())
        {
            // test regex_replace
            std::basic_string<Char> res = regex_replace(test.str, rx, test.sub, test.match_flags);
            BOOST_CHECK_MESSAGE(res == test.res, case_ << res << " != " << test.res );

            // test regex_replace with NTBS format string
            std::basic_string<Char> res2 = regex_replace(test.str, rx, test.sub.c_str(), test.match_flags);
            BOOST_CHECK_MESSAGE(res2 == test.res, case_ << res2 << " != " << test.res );

            // test regex_replace with NTBS input string
            std::basic_string<Char> res3 = regex_replace(test.str.c_str(), c_rx, test.sub, test.match_flags);
            BOOST_CHECK_MESSAGE(res3 == test.res, case_ << res3 << " != " << test.res );

            // test regex_replace with NTBS input string and NTBS format string
            std::basic_string<Char> res4 = regex_replace(test.str.c_str(), c_rx, test.sub.c_str(), test.match_flags);
            BOOST_CHECK_MESSAGE(res4 == test.res, case_ << res4 << " != " << test.res );
        }

        if(0 == (test.match_flags & regex_constants::format_first_only))
        {
            {
                // global search, use regex_iterator
                std::vector<sub_match<iterator> > br;
                regex_iterator<iterator> begin(test.str.begin(), test.str.end(), rx, test.match_flags), end;
                for(; begin != end; ++begin)
                {
                    match_results<iterator> const &what = *begin;
                    br.insert(br.end(), what.begin(), what.end());
                }

                // match succeeded: was it expected to succeed?
                BOOST_XPR_CHECK(br.size() == test.br.size());

                for(std::size_t i = 0; i < br.size() && i < test.br.size(); ++i)
                {
                    BOOST_XPR_CHECK((!br[i].matched && test.br[i] == empty) || test.br[i] == br[i].str());
                }
            }

            {
                // global search, use regex_token_iterator
                std::vector<typename sub_match<iterator>::string_type> br2;
                std::vector<int> subs(rx.mark_count() + 1, 0);
                // regex_token_iterator will extract all sub_matches, in order:
                for(std::size_t i = 0; i < subs.size(); ++i)
                {
                    subs[i] = static_cast<int>(i);
                }
                regex_token_iterator<iterator> begin2(test.str.begin(), test.str.end(), rx, subs, test.match_flags), end2;
                for(; begin2 != end2; ++begin2)
                {
                    br2.push_back(*begin2);
                }

                // match succeeded: was it expected to succeed?
                BOOST_XPR_CHECK(br2.size() == test.br.size());

                for(std::size_t i = 0; i < br2.size() && i < test.br.size(); ++i)
                {
                    BOOST_XPR_CHECK(test.br[i] == br2[i]);
                }
            }
        }
        else
        {
            // test regex_search
            match_results<iterator> what;
            if(regex_search(test.str, what, rx, test.match_flags))
            {
                // match succeeded: was it expected to succeed?
                BOOST_XPR_CHECK(what.size() == test.br.size());

                for(std::size_t i = 0; i < what.size() && i < test.br.size(); ++i)
                {
                    BOOST_XPR_CHECK((!what[i].matched && test.br[i] == empty) || test.br[i] == what[i].str());
                }
            }
            else
            {
                // match failed: was it expected to fail?
                BOOST_XPR_CHECK(0 == test.br.size());
            }
        }
    }
    catch(regex_error const &e)
    {
        BOOST_ERROR(case_ << e.what());
    }
}

///////////////////////////////////////////////////////////////////////////////
// run_test_impl
//   run the current test
void run_test()
{
    #ifdef BOOST_XPRESSIVE_TEST_WREGEX
    xpr_test_case<wchar_t> wtest = ::widen(test);
    run_test_impl(wtest);
    #else
    run_test_impl(test);
    #endif
}

///////////////////////////////////////////////////////////////////////////////
// open_test
bool open_test()
{
    // This test-file is usually run from either $BOOST_ROOT/status, or
    // $BOOST_ROOT/libs/xpressive/test. Therefore, the relative path
    // to the data file this test depends on will be one of two
    // possible paths.

    // first assume we are being run from boost_root/status
    in.open("../libs/xpressive/test/regress.txt");

    if(!in.good())
    {
        // couldn't find the data file so try to find the data file from the test dir
        in.clear();
        in.open("./regress.txt");
    }

    return in.good();
}

///////////////////////////////////////////////////////////////////////////////
// test_main
//   read the tests from the input file and execute them
void test_main()
{
    #if !defined(BOOST_XPRESSIVE_TEST_WREGEX) || !defined(BOOST_XPRESSIVE_NO_WREGEX)
    if(!open_test())
    {
        BOOST_ERROR("Error: unable to open input file.");
    }

    while(get_test())
    {
        run_test();
        ++test_count;
    }
    #endif

    std::cout << test_count << " tests completed." << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("basic regression test");
    test->add(BOOST_TEST_CASE(&test_main));
    return test;
}

///////////////////////////////////////////////////////////////////////////////
// debug_init
static const struct debug_init
{
    debug_init()
    {
    #if defined(_MSC_VER) && defined(_DEBUG)
        // Send warnings, errors and asserts to STDERR
        _CrtSetReportMode(_CRT_WARN,   _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_WARN,   _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ERROR,  _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_ERROR,  _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

        // Check for leaks at program termination
        _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));

        //_CrtSetBreakAlloc(221);
    #endif
    }
} g_debug_init;
