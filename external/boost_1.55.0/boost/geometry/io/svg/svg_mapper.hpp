// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2009-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_SVG_MAPPER_HPP
#define BOOST_GEOMETRY_IO_SVG_MAPPER_HPP

#include <cstdio>

#include <vector>

#include <boost/mpl/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>


#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/tag_cast.hpp>

#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/strategies/transform.hpp>
#include <boost/geometry/strategies/transform/map_transformer.hpp>
#include <boost/geometry/views/segment_view.hpp>

#include <boost/geometry/multi/core/tags.hpp>
#include <boost/geometry/multi/algorithms/envelope.hpp>
#include <boost/geometry/multi/algorithms/num_points.hpp>

#include <boost/geometry/io/svg/write_svg.hpp>

// Helper geometries (all points are transformed to integer-points)
#include <boost/geometry/geometries/geometries.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace svg
{
    typedef model::point<int, 2, cs::cartesian> svg_point_type;
}}
#endif


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{



template <typename GeometryTag, typename Geometry>
struct svg_map
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (Geometry)
        );
};


template <typename Point>
struct svg_map<point_tag, Point>
{
    template <typename TransformStrategy>
    static inline void apply(std::ostream& stream,
                    std::string const& style, int size,
                    Point const& point, TransformStrategy const& strategy)
    {
        detail::svg::svg_point_type ipoint;
        geometry::transform(point, ipoint, strategy);
        stream << geometry::svg(ipoint, style, size) << std::endl;
    }
};

template <typename Box>
struct svg_map<box_tag, Box>
{
    template <typename TransformStrategy>
    static inline void apply(std::ostream& stream,
                    std::string const& style, int size,
                    Box const& box, TransformStrategy const& strategy)
    {
        model::box<detail::svg::svg_point_type> ibox;
        geometry::transform(box, ibox, strategy);

        stream << geometry::svg(ibox, style, size) << std::endl;
    }
};


template <typename Range1, typename Range2>
struct svg_map_range
{
    template <typename TransformStrategy>
    static inline void apply(std::ostream& stream,
                std::string const& style, int size,
                Range1 const& range, TransformStrategy const& strategy)
    {
        Range2 irange;
        geometry::transform(range, irange, strategy);
        stream << geometry::svg(irange, style, size) << std::endl;
    }
};

template <typename Segment>
struct svg_map<segment_tag, Segment>
{
    template <typename TransformStrategy>
    static inline void apply(std::ostream& stream,
                    std::string const& style, int size,
                    Segment const& segment, TransformStrategy const& strategy)
    {
        typedef segment_view<Segment> view_type;
        view_type range(segment);
        svg_map_range
            <
                view_type,
                model::linestring<detail::svg::svg_point_type>
            >::apply(stream, style, size, range, strategy);
    }
};


template <typename Ring>
struct svg_map<ring_tag, Ring>
    : svg_map_range<Ring, model::ring<detail::svg::svg_point_type> >
{};


template <typename Linestring>
struct svg_map<linestring_tag, Linestring>
    : svg_map_range<Linestring, model::linestring<detail::svg::svg_point_type> >
{};


template <typename Polygon>
struct svg_map<polygon_tag, Polygon>
{
    template <typename TransformStrategy>
    static inline void apply(std::ostream& stream,
                    std::string const& style, int size,
                    Polygon const& polygon, TransformStrategy const& strategy)
    {
        model::polygon<detail::svg::svg_point_type> ipoly;
        geometry::transform(polygon, ipoly, strategy);
        stream << geometry::svg(ipoly, style, size) << std::endl;
    }
};


template <typename Multi>
struct svg_map<multi_tag, Multi>
{
    typedef typename single_tag_of
      <
          typename geometry::tag<Multi>::type
      >::type stag;

    template <typename TransformStrategy>
    static inline void apply(std::ostream& stream,
                    std::string const& style, int size,
                    Multi const& multi, TransformStrategy const& strategy)
    {
        for (typename boost::range_iterator<Multi const>::type it
            = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            svg_map
                <
                    stag,
                    typename boost::range_value<Multi>::type
                >::apply(stream, style, size, *it, strategy);
        }
    }
};


} // namespace dispatch
#endif


template <typename Geometry, typename TransformStrategy>
inline void svg_map(std::ostream& stream,
            std::string const& style, int size,
            Geometry const& geometry, TransformStrategy const& strategy)
{
    dispatch::svg_map
        <
            typename tag_cast
                <
                    typename tag<Geometry>::type,
                    multi_tag
                >::type,
            typename boost::remove_const<Geometry>::type
        >::apply(stream, style, size, geometry, strategy);
}


/*!
\brief Helper class to create SVG maps
\tparam Point Point type, for input geometries.
\tparam SameScale Boolean flag indicating if horizontal and vertical scale should
    be the same. The default value is true
\ingroup svg

\qbk{[include reference/io/svg.qbk]}
*/
template <typename Point, bool SameScale = true>
class svg_mapper : boost::noncopyable
{
    typedef typename geometry::select_most_precise
        <
            typename coordinate_type<Point>::type,
            double
        >::type calculation_type;

    typedef strategy::transform::map_transformer
        <
            calculation_type,
            geometry::dimension<Point>::type::value,
            geometry::dimension<Point>::type::value,
            true,
            SameScale
        > transformer_type;

    model::box<Point> m_bounding_box;
    boost::scoped_ptr<transformer_type> m_matrix;
    std::ostream& m_stream;
    int m_width, m_height;
    std::string m_width_height; // for <svg> tag only, defaults to 2x 100%

    void init_matrix()
    {
        if (! m_matrix)
        {
            m_matrix.reset(new transformer_type(m_bounding_box,
                            m_width, m_height));


            m_stream << "<?xml version=\"1.0\" standalone=\"no\"?>"
                << std::endl
                << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\""
                << std::endl
                << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">"
                << std::endl
                << "<svg " << m_width_height << " version=\"1.1\""
                << std::endl
                << "xmlns=\"http://www.w3.org/2000/svg\""
                << std::endl
                << "xmlns:xlink=\"http://www.w3.org/1999/xlink\""
                << ">"
                << std::endl;
        }
    }

public :
    
    /*!
    \brief Constructor, initializing the SVG map. Opens and initializes the SVG. 
         Should be called explicitly.
    \param stream Output stream, should be a stream already open
    \param width Width of the SVG map (in SVG pixels)
    \param height Height of the SVG map (in SVG pixels)
    \param width_height Optional information to increase width and/or height
    */
    explicit svg_mapper(std::ostream& stream, int width, int height
        , std::string const& width_height = "width=\"100%\" height=\"100%\"")
        : m_stream(stream)
        , m_width(width)
        , m_height(height)
        , m_width_height(width_height)
    {
        assign_inverse(m_bounding_box);
    }

    /*!
    \brief Destructor, called automatically. Closes the SVG by streaming <\/svg>
    */
    virtual ~svg_mapper()
    {
        m_stream << "</svg>" << std::endl;
    }

    /*!
    \brief Adds a geometry to the transformation matrix. After doing this,
        the specified geometry can be mapped fully into the SVG map
    \tparam Geometry \tparam_geometry
    \param geometry \param_geometry
    */
    template <typename Geometry>
    void add(Geometry const& geometry)
    {
        if (num_points(geometry) > 0)
        {
            expand(m_bounding_box,
                return_envelope
                    <
                        model::box<Point>
                    >(geometry));
        }
    }

    /*!
    \brief Maps a geometry into the SVG map using the specified style
    \tparam Geometry \tparam_geometry
    \param geometry \param_geometry
    \param style String containing verbatim SVG style information
    \param size Optional size (used for SVG points) in SVG pixels. For linestrings,
        specify linewidth in the SVG style information
    */
    template <typename Geometry>
    void map(Geometry const& geometry, std::string const& style,
                int size = -1)
    {
        init_matrix();
        svg_map(m_stream, style, size, geometry, *m_matrix);
    }

    /*!
    \brief Adds a text to the SVG map
    \tparam TextPoint \tparam_point
    \param point Location of the text (in map units)
    \param s The text itself
    \param style String containing verbatim SVG style information, of the text
    \param offset_x Offset in SVG pixels, defaults to 0
    \param offset_y Offset in SVG pixels, defaults to 0
    \param lineheight Line height in SVG pixels, in case the text contains \n
    */
    template <typename TextPoint>
    void text(TextPoint const& point, std::string const& s,
                std::string const& style,
                int offset_x = 0, int offset_y = 0, int lineheight = 10)
    {
        init_matrix();
        detail::svg::svg_point_type map_point;
        transform(point, map_point, *m_matrix);
        m_stream
            << "<text style=\"" << style << "\""
            << " x=\"" << get<0>(map_point) + offset_x << "\""
            << " y=\"" << get<1>(map_point) + offset_y << "\""
            << ">";
        if (s.find("\n") == std::string::npos)
        {
             m_stream  << s;
        }
        else
        {
            // Multi-line modus

            std::vector<std::string> splitted;
            boost::split(splitted, s, boost::is_any_of("\n"));
            for (std::vector<std::string>::const_iterator it
                = splitted.begin();
                it != splitted.end();
                ++it, offset_y += lineheight)
            {
                 m_stream
                    << "<tspan x=\"" << get<0>(map_point) + offset_x
                    << "\""
                    << " y=\"" << get<1>(map_point) + offset_y
                    << "\""
                    << ">" << *it << "</tspan>";
            }
        }
        m_stream << "</text>" << std::endl;
    }
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_IO_SVG_MAPPER_HPP
