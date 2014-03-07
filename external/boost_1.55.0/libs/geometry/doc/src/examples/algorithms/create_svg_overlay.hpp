// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_OVERLAY_HPP
#define CREATE_SVG_OVERLAY_HPP

#include <fstream>

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#if defined(HAVE_SVG)
#  include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

template <typename Geometry, typename Range>
void create_svg(std::string const& filename, Geometry const& a, Geometry const& b, Range const& range)
{
#if defined(HAVE_SVG)
    std::cout  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    typedef typename boost::geometry::point_type<Geometry>::type point_type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    mapper.map(b, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
    int i = 0;
    BOOST_FOREACH(typename boost::range_value<Range>::type const& g, range)
    {
        mapper.map(g, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
        std::ostringstream out;
        out << i++;
        mapper.text(boost::geometry::return_centroid<point_type>(g), out.str(),
                    "fill:rgb(0,0,0);font-family:Arial;font-size:10px");
    }
#else
    boost::ignore_unused_variable_warning(filename);
    boost::ignore_unused_variable_warning(a);
    boost::ignore_unused_variable_warning(b);
    boost::ignore_unused_variable_warning(range);
#endif    
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_OVERLAY_HPP

