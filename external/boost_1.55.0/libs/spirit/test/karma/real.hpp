/*==============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser
    Copyright (c) 2010      Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if !defined(BOOST_SPIRIT_TEST_REAL_NUMERICS_HPP)
#define BOOST_SPIRIT_TEST_REAL_NUMERICS_HPP

#include <boost/version.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/math/concepts/real_concept.hpp>

#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_numeric.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_directive.hpp>
#include <boost/spirit/include/karma_phoenix_attributes.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>

#include <boost/limits.hpp>
#include "test.hpp"

using namespace spirit_test;

///////////////////////////////////////////////////////////////////////////////
//  policy for real_generator, which forces the scientific notation
template <typename T>
struct scientific_policy : boost::spirit::karma::real_policies<T>
{
    //  we want the numbers always to be in scientific format
    typedef boost::spirit::karma::real_policies<T> base_type;
    static int floatfield(T) { return base_type::fmtflags::scientific; }
};

///////////////////////////////////////////////////////////////////////////////
//  policy for real_generator, which forces the fixed notation
template <typename T>
struct fixed_policy : boost::spirit::karma::real_policies<T>
{
    typedef boost::spirit::karma::real_policies<T> base_type;

    //  we want the numbers always to be in scientific format
    static int floatfield(T) { return base_type::fmtflags::fixed; }
};

///////////////////////////////////////////////////////////////////////////////
//  policy for real_generator, which forces to output trailing zeros in the 
//  fractional part
template <typename T>
struct trailing_zeros_policy : boost::spirit::karma::real_policies<T>   // 4 digits
{
    //  we want the numbers always to contain trailing zeros up to 4 digits in 
    //  the fractional part
    static bool trailing_zeros(T) { return true; }

    //  we want to generate up to 4 fractional digits 
    static unsigned int precision(T) { return 4; }
};

///////////////////////////////////////////////////////////////////////////////
//  policy for real_generator, which forces the sign to be generated
template <typename T>
struct signed_policy : boost::spirit::karma::real_policies<T>
{
    // we want to always have a sign generated
    static bool force_sign(T)
    {
        return true;
    }
};

///////////////////////////////////////////////////////////////////////////////
//  policy for real_generator, which forces to output trailing zeros in the 
//  fractional part
template <typename T>
struct bordercase_policy : boost::spirit::karma::real_policies<T>
{
    //  we want to generate up to the maximum significant amount of fractional 
    // digits 
    static unsigned int precision(T) 
    { 
        return std::numeric_limits<T>::digits10 + 1; 
    }
};

///////////////////////////////////////////////////////////////////////////////
//  policy for real_generator, which forces to output trailing zeros in the 
//  fractional part
template <typename T>
struct statefull_policy : boost::spirit::karma::real_policies<T>
{
    statefull_policy(int precision = 4, bool trailingzeros = false)
      : precision_(precision), trailingzeros_(trailingzeros)
    {}

    //  we want to generate up to the maximum significant amount of fractional 
    // digits 
    unsigned int precision(T) const
    { 
        return precision_; 
    }

    bool trailing_zeros(T) const
    {
        return trailingzeros_;
    }

    int precision_;
    bool trailingzeros_;
};

#endif // BOOST_SPIRIT_TEST_REAL_NUMERICS_HPP

