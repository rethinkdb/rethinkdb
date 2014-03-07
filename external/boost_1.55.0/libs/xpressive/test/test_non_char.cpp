///////////////////////////////////////////////////////////////////////////////
// test_non_char.cpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/traits/null_regex_traits.hpp>
#include "./test.hpp"

///////////////////////////////////////////////////////////////////////////////
// test_static
void test_static()
{
    static int const data[] = {0, 1, 2, 3, 4, 5, 6};
    null_regex_traits<int> nul;
    basic_regex<int const *> rex = imbue(nul)(1 >> +((set= 2,3) | 4) >> 5);
    match_results<int const *> what;

    if(!regex_search(data, data + (sizeof(data)/sizeof(*data)), what, rex))
    {
        BOOST_ERROR("regex_search on integral data failed");
    }
    else
    {
        BOOST_CHECK(*what[0].first == 1);
        BOOST_CHECK(*what[0].second == 6);
    }
}

///////////////////////////////////////////////////////////////////////////////
// UChar
struct UChar
{
    UChar(unsigned int code = 0)
      : code_(code)
    {}

    operator unsigned int () const
    {
        return this->code_;
    }

private:
    unsigned int code_;
};

///////////////////////////////////////////////////////////////////////////////
// UChar_traits
struct UChar_traits
  : null_regex_traits<UChar>
{};

///////////////////////////////////////////////////////////////////////////////
// test_dynamic
void test_dynamic()
{
    typedef std::vector<UChar>::const_iterator uchar_iterator;
    typedef basic_regex<uchar_iterator> uregex;
    typedef match_results<uchar_iterator> umatch;
    typedef regex_compiler<uchar_iterator, UChar_traits> uregex_compiler;

    std::string pattern_("b.*r"), str_("foobarboo");
    std::vector<UChar> pattern(pattern_.begin(), pattern_.end());
    std::vector<UChar> str(str_.begin(), str_.end());

    UChar_traits tr;
    uregex_compiler compiler(tr);
    uregex urx = compiler.compile(pattern);
    umatch what;

    if(!regex_search(str, what, urx))
    {
        BOOST_ERROR("regex_search on UChar failed");
    }
    else
    {
        BOOST_CHECK_EQUAL(3, what.position());
        BOOST_CHECK_EQUAL(3, what.length());
    }

    // test for range-based regex_replace
    std::vector<UChar> output = regex_replace(str, urx, pattern_);
    std::string output_(output.begin(), output.end());
    std::string expected("foob.*rboo");
    BOOST_CHECK_EQUAL(output_, expected);
}

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test_non_char");
    test->add(BOOST_TEST_CASE(&test_static));
    test->add(BOOST_TEST_CASE(&test_dynamic));
    return test;
}

