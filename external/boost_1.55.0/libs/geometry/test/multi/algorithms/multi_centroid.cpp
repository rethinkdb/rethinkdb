// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <algorithms/test_centroid.hpp>

#include <boost/geometry/multi/core/point_order.hpp>
#include <boost/geometry/multi/algorithms/centroid.hpp>
#include <boost/geometry/multi/strategies/cartesian/centroid_average.hpp>

#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/point_xy.hpp>

#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/multi/io/wkt/read.hpp>


// #define REPORT_RESULTS


template <typename P>
void test_2d(bool is_integer = false)
{
    typedef typename bg::coordinate_type<P>::type ct;
#ifdef REPORT_RESULTS
    std::cout << std::endl << "type: " << typeid(ct).name() << " size: " << sizeof(ct) << std::endl;
#endif

    if (! is_integer)
    {
        // Only working for floating point:

        test_centroid<bg::model::multi_point<P> >(
            "MULTIPOINT((1 1),(2 3),(5 0))",
            2.666666666666667, 1.33333);

        test_centroid<bg::model::multi_linestring<bg::model::linestring<P> > >(
            "MULTILINESTRING((0 0,0 2),(1 0,1 2))",
            0.5, 1.0);


        test_centroid<bg::model::multi_polygon<bg::model::polygon<P> > >(
            "MULTIPOLYGON(((1 1,1 3,3 3,3 1,1 1)),((4 1,4 3,8 3,8 1,4 1)))",
            4.666666666666667, 2.0);

        test_centroid<bg::model::multi_polygon<bg::model::polygon<P> > >(
            "MULTIPOLYGON(((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2"
            ",3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)),"
            "((10 10,10 12,12 12,12 10,10 10)))",
            7.338463104108615, 6.0606722055552407);
    }



    // Test using real-world polygon with large (Y) coordinates
    // (coordinates can be used for integer and floating point point-types)
    // Note that this will fail (overflow) if centroid calculation uses float
    test_centroid<bg::model::multi_polygon<bg::model::polygon<P> > >(
        "MULTIPOLYGON(((426062 4527794,426123 4527731"
            ",426113 4527700,426113 4527693,426115 4527671"
            ",426133 4527584,426135 4527569,426124 4527558"
            ",426103 4527547,426072 4527538,426003 4527535"
            ",425972 4527532,425950 4527531,425918 4527528"
            ",425894 4527517,425876 4527504,425870 4527484"
            ",425858 4527442,425842 4527414,425816 4527397"
            ",425752 4527384,425692 4527369,425658 4527349"
            ",425624 4527307,425605 4527260,425598 4527213"
            ",425595 4527167,425582 4527125,425548 4527064"
            ",425535 4527027,425537 4526990,425534 4526943"
            ",425525 4526904,425500 4526856,425461 4526811"
            ",425450 4526798,425381 4526823,425362 4526830"
            ",425329 4526848,425298 4526883,425291 4526897"
            ",425268 4526923,425243 4526945,425209 4526971"
            ",425172 4526990,425118 4527028,425104 4527044"
            ",425042 4527090,424980 4527126,424925 4527147"
            ",424881 4527148,424821 4527147,424698 4527125"
            ",424610 4527121,424566 4527126,424468 4527139"
            ",424426 4527141,424410 4527142,424333 4527130"
            ",424261 4527110,424179 4527073,424024 4527012"
            ",423947 4526987,423902 4526973,423858 4526961"
            ",423842 4526951,423816 4526935,423799 4526910"
            ",423776 4526905,423765 4526911,423739 4526927"
            ",423692 4526946,423636 4526976,423608 4527008"
            ",423570 4527016,423537 4527011,423505 4526996"
            ",423480 4526994,423457 4527012,423434 4527021"
            ",423367 4527008,423263 4526998,423210 4526993"
            ",423157 4526996,423110 4526994,423071 4526984"
            ",423048 4526984,423032 4526994,423254 4527613"
            ",423889 4528156,424585 4528050,425479 4527974"
            ",425795 4527867,426062 4527794)))",
        424530.6059719588, 4527519.619367547);
}



int test_main(int, char* [])
{
    test_2d<bg::model::d2::point_xy<float> >();
    test_2d<bg::model::d2::point_xy<double> >();
    test_2d<bg::model::d2::point_xy<long int> >(true);
    //test_2d<bg::model::d2::point_xy<long long> >(true);
    //test_2d<bg::model::d2::point_xy<long double> >();

#ifdef HAVE_TTMATH
    test_2d<bg::model::d2::point_xy<ttmath_big> >();
#endif

    return 0;
}
