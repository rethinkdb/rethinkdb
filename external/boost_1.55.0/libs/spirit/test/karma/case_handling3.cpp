//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// this file intentionally contains non-ascii characters
// boostinspect:noascii

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_directive.hpp>

#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    using namespace boost::spirit;

// needed for VC7.1 only
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("german")
#endif
    {
        using namespace boost::spirit::iso8859_1;

        BOOST_TEST(test("ä", lower['Ä']));
        BOOST_TEST(test("ä", lower['ä']));

        BOOST_TEST(test("Ä", upper['Ä']));
        BOOST_TEST(test("Ä", upper['ä']));
    }
#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1310))
#pragma setlocale("")
#endif

    {
        using namespace boost::spirit::ascii;

        BOOST_TEST(test("a1- ", lower["a1- "]));
        BOOST_TEST(test("a1- ", lower["a1- "]));
        BOOST_TEST(test("a1- ", lower["a1- "]));
        BOOST_TEST(test("a1- ", lower["A1- "]));

        BOOST_TEST(test("a1- ", lower[string], "a1- "));
        BOOST_TEST(test("a1- ", lower[string], "A1- "));
        BOOST_TEST(test("a1- ", lower[lit("a1- ")]));
        BOOST_TEST(test("a1- ", lower[lit("A1- ")]));
        BOOST_TEST(test("a1- ", lower[string("a1- ")]));
        BOOST_TEST(test("a1- ", lower[string("A1- ")]));

        BOOST_TEST(test("a1- ", lower[lower["a1- "]]));
        BOOST_TEST(test("a1- ", lower[lower["a1- "]]));
        BOOST_TEST(test("a1- ", lower[lower["a1- "]]));
        BOOST_TEST(test("a1- ", lower[lower["A1- "]]));

        BOOST_TEST(test("a1- ", lower[lower[string]], "a1- "));
        BOOST_TEST(test("a1- ", lower[lower[string]], "A1- "));
        BOOST_TEST(test("a1- ", lower[lower[lit("a1- ")]]));
        BOOST_TEST(test("a1- ", lower[lower[lit("A1- ")]]));
        BOOST_TEST(test("a1- ", lower[lower[string("a1- ")]]));
        BOOST_TEST(test("a1- ", lower[lower[string("A1- ")]]));

        BOOST_TEST(test("A1- ", upper[lower["a1- "]]));
        BOOST_TEST(test("A1- ", upper[lower["a1- "]]));
        BOOST_TEST(test("A1- ", upper[lower["a1- "]]));
        BOOST_TEST(test("A1- ", upper[lower["A1- "]]));

        BOOST_TEST(test("A1- ", upper[lower[string]], "a1- "));
        BOOST_TEST(test("A1- ", upper[lower[string]], "A1- "));
        BOOST_TEST(test("A1- ", upper[lower[lit("a1- ")]]));
        BOOST_TEST(test("A1- ", upper[lower[lit("A1- ")]]));
        BOOST_TEST(test("A1- ", upper[lower[string("a1- ")]]));
        BOOST_TEST(test("A1- ", upper[lower[string("A1- ")]]));

        BOOST_TEST(test("A1- ", upper["a1- "]));
        BOOST_TEST(test("A1- ", upper["a1- "]));
        BOOST_TEST(test("A1- ", upper["a1- "]));
        BOOST_TEST(test("A1- ", upper["A1- "]));

        BOOST_TEST(test("A1- ", upper[string], "a1- "));
        BOOST_TEST(test("A1- ", upper[string], "A1- "));
        BOOST_TEST(test("A1- ", upper[lit("a1- ")]));
        BOOST_TEST(test("A1- ", upper[lit("A1- ")]));

        BOOST_TEST(test("a1- ", lower[upper["a1- "]]));
        BOOST_TEST(test("a1- ", lower[upper["a1- "]]));
        BOOST_TEST(test("a1- ", lower[upper["a1- "]]));
        BOOST_TEST(test("a1- ", lower[upper["A1- "]]));

        BOOST_TEST(test("a1- ", lower[upper[string]], "a1- "));
        BOOST_TEST(test("a1- ", lower[upper[string]], "A1- "));
        BOOST_TEST(test("a1- ", lower[upper[lit("a1- ")]]));
        BOOST_TEST(test("a1- ", lower[upper[lit("A1- ")]]));
        BOOST_TEST(test("a1- ", lower[upper[string("a1- ")]]));
        BOOST_TEST(test("a1- ", lower[upper[string("A1- ")]]));

        BOOST_TEST(test("A1- ", upper[upper["a1- "]]));
        BOOST_TEST(test("A1- ", upper[upper["a1- "]]));
        BOOST_TEST(test("A1- ", upper[upper["a1- "]]));
        BOOST_TEST(test("A1- ", upper[upper["A1- "]]));

        BOOST_TEST(test("A1- ", upper[upper[string]], "a1- "));
        BOOST_TEST(test("A1- ", upper[upper[string]], "A1- "));
        BOOST_TEST(test("A1- ", upper[upper[lit("a1- ")]]));
        BOOST_TEST(test("A1- ", upper[upper[lit("A1- ")]]));
        BOOST_TEST(test("A1- ", upper[upper[string("a1- ")]]));
        BOOST_TEST(test("A1- ", upper[upper[string("A1- ")]]));
    }

    return boost::report_errors();
}
