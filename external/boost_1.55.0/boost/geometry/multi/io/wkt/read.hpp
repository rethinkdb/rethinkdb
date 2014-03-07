// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_IO_WKT_READ_MULTI_HPP
#define BOOST_GEOMETRY_MULTI_IO_WKT_READ_MULTI_HPP

#include <string>

#include <boost/geometry/core/mutable_range.hpp>
#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/core/point_type.hpp>
#include <boost/geometry/multi/geometries/concepts/check.hpp>
#include <boost/geometry/multi/io/wkt/detail/prefix.hpp>
#include <boost/geometry/io/wkt/read.hpp>

namespace boost { namespace geometry
{

namespace detail { namespace wkt
{

template <typename MultiGeometry, template<typename> class Parser, typename PrefixPolicy>
struct multi_parser
{
    static inline void apply(std::string const& wkt, MultiGeometry& geometry)
    {
        traits::clear<MultiGeometry>::apply(geometry);

        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it;
        if (initialize<MultiGeometry>(tokens, PrefixPolicy::apply(), wkt, it))
        {
            handle_open_parenthesis(it, tokens.end(), wkt);

            // Parse sub-geometries
            while(it != tokens.end() && *it != ")")
            {
                traits::resize<MultiGeometry>::apply(geometry, boost::size(geometry) + 1);
                Parser
                    <
                        typename boost::range_value<MultiGeometry>::type
                    >::apply(it, tokens.end(), wkt, *(boost::end(geometry) - 1));
                if (it != tokens.end() && *it == ",")
                {
                    // Skip "," after multi-element is parsed
                    ++it;
                }
            }

            handle_close_parenthesis(it, tokens.end(), wkt);
        }
        
        check_end(it, tokens.end(), wkt);
    }
};

template <typename P>
struct noparenthesis_point_parser
{
    static inline void apply(tokenizer::iterator& it, tokenizer::iterator end,
        std::string const& wkt, P& point)
    {
        parsing_assigner<P, 0, dimension<P>::value>::apply(it, end, point, wkt);
    }
};

template <typename MultiGeometry, typename PrefixPolicy>
struct multi_point_parser
{
    static inline void apply(std::string const& wkt, MultiGeometry& geometry)
    {
        traits::clear<MultiGeometry>::apply(geometry);

        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it;

        if (initialize<MultiGeometry>(tokens, PrefixPolicy::apply(), wkt, it))
        {
            handle_open_parenthesis(it, tokens.end(), wkt);

            // If first point definition starts with "(" then parse points as (x y)
            // otherwise as "x y"
            bool using_brackets = (it != tokens.end() && *it == "(");
            
            while(it != tokens.end() && *it != ")")
            {
                traits::resize<MultiGeometry>::apply(geometry, boost::size(geometry) + 1);
                
                if (using_brackets)
                {
                    point_parser
                        <
                            typename boost::range_value<MultiGeometry>::type
                        >::apply(it, tokens.end(), wkt, *(boost::end(geometry) - 1));
                }
                else
                {
                    noparenthesis_point_parser
                        <
                            typename boost::range_value<MultiGeometry>::type
                        >::apply(it, tokens.end(), wkt, *(boost::end(geometry) - 1));
                }
                    
                if (it != tokens.end() && *it == ",")
                {
                    // Skip "," after point is parsed
                    ++it;
                }
            }
            
            handle_close_parenthesis(it, tokens.end(), wkt);
        }
        
        check_end(it, tokens.end(), wkt);
    }
};

}} // namespace detail::wkt

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename MultiGeometry>
struct read_wkt<multi_point_tag, MultiGeometry>
    : detail::wkt::multi_point_parser
            <
                MultiGeometry,
                detail::wkt::prefix_multipoint
            >
{};

template <typename MultiGeometry>
struct read_wkt<multi_linestring_tag, MultiGeometry>
    : detail::wkt::multi_parser
            <
                MultiGeometry,
                detail::wkt::linestring_parser,
                detail::wkt::prefix_multilinestring
            >
{};

template <typename MultiGeometry>
struct read_wkt<multi_polygon_tag, MultiGeometry>
    : detail::wkt::multi_parser
            <
                MultiGeometry,
                detail::wkt::polygon_parser,
                detail::wkt::prefix_multipolygon
            >
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_IO_WKT_READ_MULTI_HPP
