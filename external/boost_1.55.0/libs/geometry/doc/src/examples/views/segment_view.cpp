// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[segment_view
//` Shows usage of the Boost.Range compatible view on a box

#include <iostream>

#include <boost/geometry.hpp>


int main()
{
    typedef boost::geometry::model::segment
        <
            boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>
        > segment_type;
        
    typedef boost::geometry::segment_view<segment_type> segment_view;        
        
    segment_type segment;
    boost::geometry::assign_values(segment, 0, 0, 1, 1);
    
    segment_view view(segment);
    
    // Iterating over the points of this segment
    for (boost::range_iterator<segment_view const>::type it = boost::begin(view);
        it != boost::end(view); ++it)
    {
        std::cout << " " << boost::geometry::dsv(*it);
    }
    std::cout << std::endl;
    
    // Note that a segment_view is tagged as a linestring, so supports length etc.
    std::cout << "Length: " << boost::geometry::length(view) << std::endl;
    
    return 0;
}

//]


//[segment_view_output
/*`
Output:
[pre
 (0, 0) (0, 4) (4, 4) (4, 0) (0, 0)
Area: 16
]
*/
//]
