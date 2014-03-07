// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_WKT_READ_HPP
#define BOOST_GEOMETRY_IO_WKT_READ_HPP

#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/mpl/if.hpp>
#include <boost/range.hpp>

#include <boost/type_traits.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/clear.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/exception.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/geometry_id.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/mutable_range.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/coordinate_cast.hpp>

#include <boost/geometry/io/wkt/detail/prefix.hpp>

namespace boost { namespace geometry
{

/*!
\brief Exception showing things wrong with WKT parsing
\ingroup wkt
*/
struct read_wkt_exception : public geometry::exception
{
    template <typename Iterator>
    read_wkt_exception(std::string const& msg,
            Iterator const& it, Iterator const& end, std::string const& wkt)
        : message(msg)
        , wkt(wkt)
    {
        if (it != end)
        {
            source = " at '";
            source += it->c_str();
            source += "'";
        }
        complete = message + source + " in '" + wkt.substr(0, 100) + "'";
    }

    read_wkt_exception(std::string const& msg, std::string const& wkt)
        : message(msg)
        , wkt(wkt)
    {
        complete = message + "' in (" + wkt.substr(0, 100) + ")";
    }

    virtual ~read_wkt_exception() throw() {}

    virtual const char* what() const throw()
    {
        return complete.c_str();
    }
private :
    std::string source;
    std::string message;
    std::string wkt;
    std::string complete;
};


#ifndef DOXYGEN_NO_DETAIL
// (wkt: Well Known Text, defined by OGC for all geometries and implemented by e.g. databases (MySQL, PostGIS))
namespace detail { namespace wkt
{

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

template <typename Point, std::size_t Dimension, std::size_t DimensionCount>
struct parsing_assigner
{
    static inline void apply(tokenizer::iterator& it, tokenizer::iterator end,
            Point& point, std::string const& wkt)
    {
        typedef typename coordinate_type<Point>::type coordinate_type;

        // Stop at end of tokens, or at "," ot ")"
        bool finished = (it == end || *it == "," || *it == ")");

        try
        {
            // Initialize missing coordinates to default constructor (zero)
            // OR
            // Use lexical_cast for conversion to double/int
            // Note that it is much slower than atof. However, it is more standard
            // and in parsing the change in performance falls probably away against
            // the tokenizing
            set<Dimension>(point, finished
                    ? coordinate_type()
                    : coordinate_cast<coordinate_type>::apply(*it));
        }
        catch(boost::bad_lexical_cast const& blc)
        {
            throw read_wkt_exception(blc.what(), it, end, wkt);
        }
        catch(std::exception const& e)
        {
            throw read_wkt_exception(e.what(), it, end, wkt);
        }
        catch(...)
        {
            throw read_wkt_exception("", it, end, wkt);
        }

        parsing_assigner<Point, Dimension + 1, DimensionCount>::apply(
                        (finished ? it : ++it), end, point, wkt);
    }
};

template <typename Point, std::size_t DimensionCount>
struct parsing_assigner<Point, DimensionCount, DimensionCount>
{
    static inline void apply(tokenizer::iterator&, tokenizer::iterator, Point&,
                std::string const&)
    {
    }
};



template <typename Iterator>
inline void handle_open_parenthesis(Iterator& it,
            Iterator const& end, std::string const& wkt)
{
    if (it == end || *it != "(")
    {
        throw read_wkt_exception("Expected '('", it, end, wkt);
    }
    ++it;
}


template <typename Iterator>
inline void handle_close_parenthesis(Iterator& it,
            Iterator const& end, std::string const& wkt)
{
    if (it != end && *it == ")")
    {
        ++it;
    }
    else
    {
        throw read_wkt_exception("Expected ')'", it, end, wkt);
    }
}

template <typename Iterator>
inline void check_end(Iterator& it,
            Iterator const& end, std::string const& wkt)
{
    if (it != end)
    {
        throw read_wkt_exception("Too much tokens", it, end, wkt);
    }
}

/*!
\brief Internal, parses coordinate sequences, strings are formated like "(1 2,3 4,...)"
\param it token-iterator, should be pre-positioned at "(", is post-positions after last ")"
\param end end-token-iterator
\param out Output itererator receiving coordinates
*/
template <typename Point>
struct container_inserter
{
    // Version with output iterator
    template <typename OutputIterator>
    static inline void apply(tokenizer::iterator& it, tokenizer::iterator end,
        std::string const& wkt, OutputIterator out)
    {
        handle_open_parenthesis(it, end, wkt);

        Point point;

        // Parse points until closing parenthesis

        while (it != end && *it != ")")
        {
            parsing_assigner
                <
                    Point,
                    0,
                    dimension<Point>::value
                >::apply(it, end, point, wkt);
            out = point;
            ++out;
            if (it != end && *it == ",")
            {
                ++it;
            }
        }

        handle_close_parenthesis(it, end, wkt);
    }
};


// Geometry is a value-type or reference-type
template <typename Geometry>
struct container_appender
{
    typedef typename geometry::point_type
        <
            typename boost::remove_reference<Geometry>::type
        >::type point_type;

    static inline void apply(tokenizer::iterator& it, tokenizer::iterator end,
        std::string const& wkt, Geometry out)
    {
        handle_open_parenthesis(it, end, wkt);

        point_type point;

        // Parse points until closing parenthesis

        while (it != end && *it != ")")
        {
            parsing_assigner
                <
                    point_type,
                    0,
                    dimension<point_type>::value
                >::apply(it, end, point, wkt);

            geometry::append(out, point);
            if (it != end && *it == ",")
            {
                ++it;
            }
        }

        handle_close_parenthesis(it, end, wkt);
    }
};

/*!
\brief Internal, parses a point from a string like this "(x y)"
\note used for parsing points and multi-points
*/
template <typename P>
struct point_parser
{
    static inline void apply(tokenizer::iterator& it, tokenizer::iterator end,
        std::string const& wkt, P& point)
    {
        handle_open_parenthesis(it, end, wkt);
        parsing_assigner<P, 0, dimension<P>::value>::apply(it, end, point, wkt);
        handle_close_parenthesis(it, end, wkt);
    }
};


template <typename Geometry>
struct linestring_parser
{
    static inline void apply(tokenizer::iterator& it, tokenizer::iterator end,
                std::string const& wkt, Geometry& geometry)
    {
        container_appender<Geometry&>::apply(it, end, wkt, geometry);
    }
};


template <typename Ring>
struct ring_parser
{
    static inline void apply(tokenizer::iterator& it, tokenizer::iterator end,
                std::string const& wkt, Ring& ring)
    {
        // A ring should look like polygon((x y,x y,x y...))
        // So handle the extra opening/closing parentheses
        // and in between parse using the container-inserter
        handle_open_parenthesis(it, end, wkt);
        container_appender<Ring&>::apply(it, end, wkt, ring);
        handle_close_parenthesis(it, end, wkt);
    }
};




/*!
\brief Internal, parses a polygon from a string like this "((x y,x y),(x y,x y))"
\note used for parsing polygons and multi-polygons
*/
template <typename Polygon>
struct polygon_parser
{
    typedef typename ring_return_type<Polygon>::type ring_return_type;
    typedef container_appender<ring_return_type> appender;

    static inline void apply(tokenizer::iterator& it, tokenizer::iterator end,
                std::string const& wkt, Polygon& poly)
    {

        handle_open_parenthesis(it, end, wkt);

        int n = -1;

        // Stop at ")"
        while (it != end && *it != ")")
        {
            // Parse ring
            if (++n == 0)
            {
                appender::apply(it, end, wkt, exterior_ring(poly));
            }
            else
            {
                typename ring_type<Polygon>::type ring;
                appender::apply(it, end, wkt, ring);
                traits::push_back
                    <
                        typename boost::remove_reference
                        <
                            typename traits::interior_mutable_type<Polygon>::type
                        >::type
                    >::apply(interior_rings(poly), ring);
            }

            if (it != end && *it == ",")
            {
                // Skip "," after ring is parsed
                ++it;
            }
        }

        handle_close_parenthesis(it, end, wkt);
    }
};

inline bool one_of(tokenizer::iterator const& it, std::string const& value,
            bool& is_present)
{
    if (boost::iequals(*it, value))
    {
        is_present = true;
        return true;
    }
    return false;
}

inline bool one_of(tokenizer::iterator const& it, std::string const& value,
            bool& present1, bool& present2)
{
    if (boost::iequals(*it, value))
    {
        present1 = true;
        present2 = true;
        return true;
    }
    return false;
}


inline void handle_empty_z_m(tokenizer::iterator& it, tokenizer::iterator end,
            bool& has_empty, bool& has_z, bool& has_m)
{
    has_empty = false;
    has_z = false;
    has_m = false;

    // WKT can optionally have Z and M (measured) values as in
    // POINT ZM (1 1 5 60), POINT M (1 1 80), POINT Z (1 1 5)
    // GGL supports any of them as coordinate values, but is not aware
    // of any Measured value.
    while (it != end
           && (one_of(it, "M", has_m)
               || one_of(it, "Z", has_z)
               || one_of(it, "EMPTY", has_empty)
               || one_of(it, "MZ", has_m, has_z)
               || one_of(it, "ZM", has_z, has_m)
               )
           )
    {
        ++it;
    }
}

/*!
\brief Internal, starts parsing
\param tokens boost tokens, parsed with separator " " and keeping separator "()"
\param geometry string to compare with first token
*/
template <typename Geometry>
inline bool initialize(tokenizer const& tokens,
            std::string const& geometry_name, std::string const& wkt,
            tokenizer::iterator& it)
{
    it = tokens.begin();
    if (it != tokens.end() && boost::iequals(*it++, geometry_name))
    {
        bool has_empty, has_z, has_m;

        handle_empty_z_m(it, tokens.end(), has_empty, has_z, has_m);

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)  
#pragma warning(disable : 4127)  
#endif

        if (has_z && dimension<Geometry>::type::value < 3)
        {
            throw read_wkt_exception("Z only allowed for 3 or more dimensions", wkt);
        }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

        if (has_empty)
        {
            check_end(it, tokens.end(), wkt);
            return false;
        }
        // M is ignored at all.

        return true;
    }
    throw read_wkt_exception(std::string("Should start with '") + geometry_name + "'", wkt);
}


template <typename Geometry, template<typename> class Parser, typename PrefixPolicy>
struct geometry_parser
{
    static inline void apply(std::string const& wkt, Geometry& geometry)
    {
        geometry::clear(geometry);

        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it;
        if (initialize<Geometry>(tokens, PrefixPolicy::apply(), wkt, it))
        {
            Parser<Geometry>::apply(it, tokens.end(), wkt, geometry);
            check_end(it, tokens.end(), wkt);
        }
    }
};





/*!
\brief Supports box parsing
\note OGC does not define the box geometry, and WKT does not support boxes.
    However, to be generic GGL supports reading and writing from and to boxes.
    Boxes are outputted as a standard POLYGON. GGL can read boxes from
    a standard POLYGON, from a POLYGON with 2 points of from a BOX
\tparam Box the box
*/
template <typename Box>
struct box_parser
{
    static inline void apply(std::string const& wkt, Box& box)
    {
        bool should_close = false;
        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it = tokens.begin();
        tokenizer::iterator end = tokens.end();
        if (it != end && boost::iequals(*it, "POLYGON"))
        {
            ++it;
            bool has_empty, has_z, has_m;
            handle_empty_z_m(it, end, has_empty, has_z, has_m);
            if (has_empty)
            {
                assign_zero(box);
                return;
            }
            handle_open_parenthesis(it, end, wkt);
            should_close = true;
        }
        else if (it != end && boost::iequals(*it, "BOX"))
        {
            ++it;
        }
        else
        {
            throw read_wkt_exception("Should start with 'POLYGON' or 'BOX'", wkt);
        }

        typedef typename point_type<Box>::type point_type;
        std::vector<point_type> points;
        container_inserter<point_type>::apply(it, end, wkt, std::back_inserter(points));

        if (should_close)
        {
            handle_close_parenthesis(it, end, wkt);
        }
        check_end(it, end, wkt);

        int index = 0;
        int n = boost::size(points);
        if (n == 2)
        {
            index = 1;
        }
        else if (n == 4 || n == 5)
        {
            // In case of 4 or 5 points, we do not check the other ones, just
            // take the opposite corner which is always 2
            index = 2;
        }
        else
        {
            throw read_wkt_exception("Box should have 2,4 or 5 points", wkt);
        }

        geometry::detail::assign_point_to_index<min_corner>(points.front(), box);
        geometry::detail::assign_point_to_index<max_corner>(points[index], box);
    }
};


/*!
\brief Supports segment parsing
\note OGC does not define the segment, and WKT does not support segmentes.
    However, it is useful to implement it, also for testing purposes
\tparam Segment the segment
*/
template <typename Segment>
struct segment_parser
{
    static inline void apply(std::string const& wkt, Segment& segment)
    {
        tokenizer tokens(wkt, boost::char_separator<char>(" ", ",()"));
        tokenizer::iterator it = tokens.begin();
        tokenizer::iterator end = tokens.end();
        if (it != end &&
            (boost::iequals(*it, "SEGMENT")
            || boost::iequals(*it, "LINESTRING") ))
        {
            ++it;
        }
        else
        {
            throw read_wkt_exception("Should start with 'LINESTRING' or 'SEGMENT'", wkt);
        }

        typedef typename point_type<Segment>::type point_type;
        std::vector<point_type> points;
        container_inserter<point_type>::apply(it, end, wkt, std::back_inserter(points));

        check_end(it, end, wkt);

        if (boost::size(points) == 2)
        {
            geometry::detail::assign_point_to_index<0>(points.front(), segment);
            geometry::detail::assign_point_to_index<1>(points.back(), segment);
        }
        else
        {
            throw read_wkt_exception("Segment should have 2 points", wkt);
        }

    }
};



}} // namespace detail::wkt
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Tag, typename Geometry>
struct read_wkt {};


template <typename Point>
struct read_wkt<point_tag, Point>
    : detail::wkt::geometry_parser
        <
            Point,
            detail::wkt::point_parser,
            detail::wkt::prefix_point
        >
{};


template <typename L>
struct read_wkt<linestring_tag, L>
    : detail::wkt::geometry_parser
        <
            L,
            detail::wkt::linestring_parser,
            detail::wkt::prefix_linestring
        >
{};

template <typename Ring>
struct read_wkt<ring_tag, Ring>
    : detail::wkt::geometry_parser
        <
            Ring,
            detail::wkt::ring_parser,
            detail::wkt::prefix_polygon
        >
{};

template <typename Geometry>
struct read_wkt<polygon_tag, Geometry>
    : detail::wkt::geometry_parser
        <
            Geometry,
            detail::wkt::polygon_parser,
            detail::wkt::prefix_polygon
        >
{};


// Box (Non-OGC)
template <typename Box>
struct read_wkt<box_tag, Box>
    : detail::wkt::box_parser<Box>
{};

// Segment (Non-OGC)
template <typename Segment>
struct read_wkt<segment_tag, Segment>
    : detail::wkt::segment_parser<Segment>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

/*!
\brief Parses OGC Well-Known Text (\ref WKT) into a geometry (any geometry)
\ingroup wkt
\param wkt string containing \ref WKT
\param geometry output geometry
\par Example:
\note It is case insensitive and can have the WKT forms "point", "point m", "point z", "point zm", "point mz"
\note Empty sequences can have forms as "LINESTRING ()" or "POLYGON(())"
Small example showing how to use read_wkt to build a point
\dontinclude doxygen_1.cpp
\skip example_from_wkt_point
\line {
\until }
\par Example:
Small example showing how to use read_wkt to build a linestring
\dontinclude doxygen_1.cpp
\skip example_from_wkt_linestring
\line {
\until }
\par Example:
Small example showing how to use read_wkt to build a polygon
\dontinclude doxygen_1.cpp
\skip example_from_wkt_polygon
\line {
\until }
*/
template <typename Geometry>
inline void read_wkt(std::string const& wkt, Geometry& geometry)
{
    geometry::concept::check<Geometry>();
    dispatch::read_wkt<typename tag<Geometry>::type, Geometry>::apply(wkt, geometry);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_IO_WKT_READ_HPP
