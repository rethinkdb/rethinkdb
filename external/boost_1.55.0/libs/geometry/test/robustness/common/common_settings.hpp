// Boost.Geometry (aka GGL, Generic Geometry Library)
// Robustness Test
//
// Copyright (c) 2009-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_COMMON_SETTINGS_HPP
#define BOOST_GEOMETRY_COMMON_SETTINGS_HPP

struct common_settings
{
    bool svg;
    bool wkt;
    bool also_difference;
    double tolerance;
    
    int field_size;
    bool triangular;

    common_settings()
        : svg(false)
        , wkt(false)
        , also_difference(false)
        , tolerance(1.0e-6)
        , field_size(10)
        , triangular(false)
    {}
};

#endif // BOOST_GEOMETRY_COMMON_SETTINGS_HPP
