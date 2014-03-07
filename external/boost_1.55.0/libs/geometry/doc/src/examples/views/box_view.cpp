// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[box_view
//` Shows usage of the Boost.Range compatible view on a box

#include <iostream>

#include <boost/geometry.hpp>


int main()
{
    typedef boost::geometry::model::box
        <
            boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>
        > box_type;
        
    // Define the Boost.Range compatible type:
    typedef boost::geometry::box_view<box_type> box_view;        
        
    box_type box;
    boost::geometry::assign_values(box, 0, 0, 4, 4);
    
    box_view view(box);
    
    // Iterating in clockwise direction over the points of this box
    for (boost::range_iterator<box_view const>::type it = boost::begin(view);
        it != boost::end(view); ++it)
    {
        std::cout << " " << boost::geometry::dsv(*it);
    }
    std::cout << std::endl;
    
    // Note that a box_view is tagged as a ring, so supports area etc.
    std::cout << "Area: " << boost::geometry::area(view) << std::endl;
    
    return 0;
}

//]


//[box_view_output
/*`
Output:
[pre
 (0, 0) (0, 4) (4, 4) (4, 0) (0, 0)
Area: 16
]
*/
//]
