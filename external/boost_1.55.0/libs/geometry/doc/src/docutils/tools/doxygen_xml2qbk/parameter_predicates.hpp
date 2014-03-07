// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2010-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
#ifndef PARAMETER_PREDICATES_HPP
#define PARAMETER_PREDICATES_HPP


#include <string>

#include <doxygen_elements.hpp>


// Predicate for std::find_if
struct par_by_name
{
    par_by_name(std::string const& n)
        : m_name(n)
    {}

    inline bool operator()(parameter const& p)
    {
        return p.name == m_name;
    }
private :
    std::string m_name;
};


// Predicate for std::find_if
struct par_by_type
{
    par_by_type(std::string const& n)
        : m_type(n)
    {}

    inline bool operator()(parameter const& p)
    {
        return p.type == m_type;
    }
private :
    std::string m_type;
};


template <typename Element>
struct sort_on_line
{
    inline bool operator()(Element const& left, Element const& right)
    {
        return left.line < right.line;
    }
};


#endif // PARAMETER_PREDICATES_HPP
