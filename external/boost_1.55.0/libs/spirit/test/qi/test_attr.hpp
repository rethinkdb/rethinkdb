//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_PP_IS_ITERATING)

#if !defined(BOOST_SPIRIT_QI_TEST_ATTR_APR_23_2009_0605PM)
#define BOOST_SPIRIT_QI_TEST_ATTR_APR_23_2009_0605PM

#include <cstring>
#include <string>
#include <iterator>
#include <iostream>
#include <typeinfo>

#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_what.hpp>

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/iterate.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

#define BOOST_PP_FILENAME_1 "qi/test_attr.hpp"
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
#define DEFINE_ATTRIBUTE(z, n, _)                                             \
    BOOST_PP_CAT(A, n) BOOST_PP_CAT(attr, n) = BOOST_PP_CAT(A, n)();
#define COMPARE_ATTRIBUTE(z, n, _)                                            \
    BOOST_PP_CAT(attr, n) == BOOST_PP_CAT(val, n) &&

namespace spirit_test
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Parser
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool test(Char const *in, Parser const& p
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, val))
    {
        namespace qi = boost::spirit::qi;

        // we don't care about the result of the "what" function.
        // we only care that all parsers have it:
        qi::what(p);

        Char const* last = in;
        while (*last)
            last++;

        BOOST_PP_REPEAT(N, DEFINE_ATTRIBUTE, _);
        bool result = qi::parse(in, last, p, BOOST_PP_ENUM_PARAMS(N, attr));

        return result && BOOST_PP_REPEAT(N, COMPARE_ATTRIBUTE, _) in == last;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Parser, typename Skipper
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool test_skipped(Char const *in, Parser const& p
      , Skipper const& skipper, BOOST_PP_ENUM_BINARY_PARAMS(N, A, val))
    {
        namespace qi = boost::spirit::qi;

        // we don't care about the result of the "what" function.
        // we only care that all parsers have it:
        qi::what(p);

        Char const* last = in;
        while (*last)
            last++;

        BOOST_PP_REPEAT(N, DEFINE_ATTRIBUTE, _);
        bool result = qi::phrase_parse(in, last, p, skipper
          , BOOST_PP_ENUM_PARAMS(N, attr));

        return result && BOOST_PP_REPEAT(N, COMPARE_ATTRIBUTE, _) in == last;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Parser, typename Skipper
      , BOOST_PP_ENUM_PARAMS(N, typename A)>
    inline bool test_postskipped(Char const *in, Parser const& p
      , Skipper const& skipper
      , BOOST_SCOPED_ENUM(boost::spirit::qi::skip_flag) post_skip
      , BOOST_PP_ENUM_BINARY_PARAMS(N, A, val))
    {
        namespace qi = boost::spirit::qi;

        // we don't care about the result of the "what" function.
        // we only care that all parsers have it:
        qi::what(p);

        Char const* last = in;
        while (*last)
            last++;

        BOOST_PP_REPEAT(N, DEFINE_ATTRIBUTE, _);
        bool result = qi::phrase_parse(in, last, p, skipper, post_skip
          , BOOST_PP_ENUM_PARAMS(N, attr));

        return result && BOOST_PP_REPEAT(N, COMPARE_ATTRIBUTE, _) in == last;
    }

}   // namespace spirit_test

#undef COMPARE_ATTRIBUTE
#undef DEFINE_ATTRIBUTE
#undef N

#endif 
