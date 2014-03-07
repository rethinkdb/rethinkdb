//  Copyright (c) 2013 Louis Dionne
//  Copyright (c) 2001-2013 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/spirit/include/karma.hpp>

#include <iostream>

// Note how the return is made by value instead of by reference.
template <typename T> T identity(T const& t) { return t; }

template <typename Char, typename Expr, typename CopyExpr, typename CopyAttr
  , typename Delimiter, typename Attribute>
bool test(Char const *expected, 
    boost::spirit::karma::detail::format_manip<
        Expr, CopyExpr, CopyAttr, Delimiter, Attribute> const& fm)
{
    std::ostringstream ostrm;
    ostrm << fm;
    return ostrm.good() && ostrm.str() == expected;
}

int main() 
{
    namespace karma = boost::spirit::karma;
    namespace adaptors = boost::adaptors;
    int ints[] = {0, 1, 2, 3, 4};

    BOOST_TEST((test("0 1 2 3 4", 
        karma::format(karma::int_ % ' ', 
            ints | adaptors::transformed(&identity<int>)))
    ));

    return boost::report_errors();
}

