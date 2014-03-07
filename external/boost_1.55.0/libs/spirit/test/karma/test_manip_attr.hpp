//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_PP_IS_ITERATING)

#if !defined(BOOST_SPIRIT_KARMA_TEST_MANIP_ATTR_APR_24_2009_0834AM)
#define BOOST_SPIRIT_KARMA_TEST_MANIP_ATTR_APR_24_2009_0834AM

#include <cstring>
#include <string>
#include <iterator>
#include <iostream>
#include <typeinfo>

#include <boost/spirit/include/karma_stream.hpp>

#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

namespace spirit_test
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename T>
    void print_if_failed(char const* func, bool result
      , std::basic_string<Char> const& generated, T const& expected)
    {
        if (!result)
            std::cerr << "in " << func << ": result is false" << std::endl;
        else if (generated != expected)
            std::cerr << "in " << func << ": generated \""
                << std::string(generated.begin(), generated.end())
                << "\"" << std::endl;
    }
}

#define BOOST_PP_FILENAME_1 "karma/test_manip_attr.hpp"
#define BOOST_PP_ITERATION_LIMITS (1, SPIRIT_ARGUMENTS_LIMIT)
#include BOOST_PP_ITERATE()

#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Preprocessor vertical repetition code
//
///////////////////////////////////////////////////////////////////////////////
#else // defined(BOOST_PP_IS_ITERATING)

#define N BOOST_PP_ITERATION()

namespace spirit_test
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Generator
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool test(Char const *expected, Generator const& g
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        namespace karma = boost::spirit::karma;

        std::ostringstream ostrm;
        ostrm << karma::format(g, BOOST_PP_ENUM_PARAMS(N, attr));

        print_if_failed("test", ostrm.good(), ostrm.str(), expected);
        return ostrm.good() && ostrm.str() == expected;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Generator, typename Delimiter
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool test_delimited(Char const *expected, Generator const& g
      , Delimiter const& d, BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        namespace karma = boost::spirit::karma;

        std::ostringstream ostrm;
        ostrm << karma::format_delimited(g, d, BOOST_PP_ENUM_PARAMS(N, attr));

        print_if_failed("test_delimited", ostrm.good(), ostrm.str(), expected);
        return ostrm.good() && ostrm.str() == expected;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Generator, typename Delimiter
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool test_predelimited(Char const *expected, Generator const& g
      , Delimiter const& d
      , BOOST_SCOPED_ENUM(boost::spirit::karma::delimit_flag) pre_delimit
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, const& attr))
    {
        namespace karma = boost::spirit::karma;

        std::ostringstream ostrm;
        ostrm << karma::format_delimited(g, d, pre_delimit
          , BOOST_PP_ENUM_PARAMS(N, attr));

        print_if_failed("test_predelimited", ostrm.good(), ostrm.str(), expected);
        return ostrm.good() && ostrm.str() == expected;
    }

}   // namespace spirit_test

#undef N

#endif 
