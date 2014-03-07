// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2010-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_TEST_MULTI_OVERLAY_CASES_HPP
#define BOOST_GEOMETRY_TEST_MULTI_OVERLAY_CASES_HPP


#include <string>


static std::string case_multi_simplex[2] = {
        "MULTIPOLYGON(((0 1,2 5,5 3,0 1)),((1 1,5 2,5 0,1 1)))",
        "MULTIPOLYGON(((3 0,0 3,4 5,3 0)))" };

// To mix multi/single 
static std::string case_single_simplex = "POLYGON((3 0,0 3,4 5,3 0))";

static std::string case_multi_no_ip[2] = 
{
    "MULTIPOLYGON(((4 1,0 7,7 9,4 1)),((8 1,6 3,10 4,8 1)),((12 6,10 7,13 8,12 6)))",
    "MULTIPOLYGON(((14 4,8 8,15 10,14 4)),((15 3,18 9,20 2,15 3)),((3 4,1 7,5 7,3 4)))" 
};

static std::string case_multi_2[2] = 
{
    "MULTIPOLYGON(((4 3,2 7,10 9,4 3)),((8 1,6 3,10 4,8 1)),((12 6,10 7,13 8,12 6)))",
    "MULTIPOLYGON(((14 4,8 8,15 10,14 4)),((15 3,18 9,20 2,15 3)),((5 5,4 7,7 7,5 5)))" 
};

static std::string case_61_multi[2] = 
{
    // extracted from recursive boxes
    "MULTIPOLYGON(((1 1,1 2,2 2,2 1,1 1)),((2 2,2 3,3 3,3 2,2 2)))",
    "MULTIPOLYGON(((1 2,1 3,2 3,2 2,1 2)),((2 3,2 4,3 4,3 3,2 3)))" 
};

static std::string case_62_multi[2] = 
{
    // extracted from recursive boxes
    "MULTIPOLYGON(((1 2,1 3,2 3,2 2,1 2)))",
    "MULTIPOLYGON(((1 2,1 3,2 3,2 2,1 2)),((2 3,2 4,3 4,3 3,2 3)))" 
};

static std::string case_63_multi[2] = 
{
    // extracted from recursive boxes
    "MULTIPOLYGON(((1 2,1 3,2 3,2 2,1 2)))",
    "MULTIPOLYGON(((1 2,1 3,2 3,2 2,1 2)),((2 1,2 2,3 2,3 1,2 1)))" 
};

static std::string case_64_multi[3] = 
{
    // extracted from recursive boxes
    "MULTIPOLYGON(((1 1,1 2,2 2,2 1,1 1)),((2 2,2 3,3 3,3 2,2 2)))",
    "MULTIPOLYGON(((1 1,1 2,2 2,3 2,3 1,2 1,1 1)))" ,
    // same but omitting not-necessary form-points at x=2 (==simplified)
    "MULTIPOLYGON(((1 1,1 2,3 2,3 1,1 1)))" 
};

static std::string case_65_multi[2] = 
{
    "MULTIPOLYGON(((2 2,2 3,3 3,3 2,2 2)))",
    "MULTIPOLYGON(((1 1,1 2,2 2,2 1,1 1)),((2 2,2 3,3 3,3 2,2 2)),((3 1,3 2,5 2,5 1,3 1)))" 
};

static std::string case_66_multi[2] = 
{
    "MULTIPOLYGON(((3 5,2 5,2 6,3 6,4 6,4 5,3 5)),((1 6,0 6,0 7,1 7,2 7,2 6,1 6)))",
    "MULTIPOLYGON(((1 4,1 5,2 5,2 4,1 4)),((1 7,2 7,2 6,1 6,1 7)),((0 8,0 9,1 9,1 8,1 7,0 7,0 8)))" 
};

static std::string case_67_multi[2] = 
{
    "MULTIPOLYGON(((1 2,1 3,2 3,2 2,1 2)),((2 1,2 2,3 2,3 1,2 1)))",
    "MULTIPOLYGON(((1 1,1 2,3 2,3 1,1 1)))" 
};

static std::string case_68_multi[2] = 
{
    "MULTIPOLYGON(((2 1,2 2,4 2,4 1,2 1)),((4 2,4 3,5 3,5 2,4 2)))",
    "MULTIPOLYGON(((1 2,1 3,2 3,2 2,1 2)),((2 1,2 2,3 2,3 1,2 1)),((3 2,3 3,5 3,5 2,3 2)))" 
};

static std::string case_69_multi[2] = 
{
    "MULTIPOLYGON(((1 1,1 2,2 2,2 1,1 1)),((3 2,3 3,4 3,4 2,3 2)))",
    "MULTIPOLYGON(((2 0,2 1,3 1,3 0,2 0)),((1 1,1 3,2 3,2 1,1 1)),((2 3,2 4,3 4,3 3,2 3)))" 
};

static std::string case_71_multi[2] = 
{
    "MULTIPOLYGON(((0 0,0 3,1 3,1 1,3 1,3 2,4 2,4 0,0 0)),((2 2,2 3,3 3,3 2,2 2)))",
    "MULTIPOLYGON(((0 2,0 3,3 3,3 2,0 2)))" 
};

static std::string case_72_multi[2] = 
{
    // cluster with ii, done by both traverse and assemble
    "MULTIPOLYGON(((0 3,4 4,3 0,3 3,0 3)),((3 3,2 1,1 2,3 3)))",
    "MULTIPOLYGON(((0 0,1 4,3 3,4 1,0 0)))" 
};

static std::string case_73_multi[2] = 
{
    "MULTIPOLYGON(((2 2,2 3,3 3,3 2,2 2)),((1 1,1 2,2 2,2 1,1 1)))",
    "MULTIPOLYGON(((1 1,1 2,2 2,2 3,3 3,3 1,1 1)))" 
};

static std::string case_74_multi[2] = 
{
    "MULTIPOLYGON(((3 0,2 0,2 1,3 1,3 3,1 3,1 2,2 2,2 1,0 1,0 5,4 5,4 0,3 0)))",
    "MULTIPOLYGON(((0 2,0 3,1 3,1 1,2 1,2 0,0 0,0 2)),((2 3,1 3,1 4,2 4,2 3)))"
};
 
static std::string case_75_multi[2] = 
{
    // cc/uu turns on all corners of second box
    "MULTIPOLYGON(((1 1,1 2,2 2,2 1,1 1)),((1 3,1 4,2 4,2 3,1 3)),((2 2,2 3,3 3,3 2,2 2)),((3 1,3 2,4 2,4 1,3 1)),((3 3,3 4,4 4,4 3,3 3)))",
    "MULTIPOLYGON(((2 2,2 3,3 3,3 2,2 2)))"
};

static std::string case_76_multi[2] = 
{
    // cc/uu turns on all corners of second box, might generate TWO OVERLAPPING union polygons!
    // therefore, don't follow uu.
    "MULTIPOLYGON(((1 0,1 1,2 1,2 0,1 0)),((3 2,4 2,4 1,3 1,3 2)),((2 2,2 3,3 3,3 2,2 2)),((2 3,1 3,1 4,2 4,2 3)),((3 3,3 4,4 4,4 3,3 3)))",
    "MULTIPOLYGON(((0 2,0 3,1 3,1 2,2 2,2 0,1 0,1 1,0 1,0 2)),((2 2,2 3,3 3,3 2,2 2)))" 
};

static std::string case_77_multi[2] = 
{
    // with a point on interior-ring-border of enclosing 
    // -> went wrong in the assemble phase for intersection (traversal is OK)
    // -> fixed
    "MULTIPOLYGON(((3 3,3 4,4 4,4 3,3 3)),((5 3,5 4,4 4,4 5,3 5,3 6,5 6,5 5,7 5,7 6,8 6,8 5,9 5,9 2,8 2,8 1,7 1,7 2,5 2,5 3),(6 3,8 3,8 4,6 4,6 3)))",
    "MULTIPOLYGON(((6 3,6 4,7 4,7 3,6 3)),((2 3,1 3,1 4,3 4,3 5,4 5,4 6,5 6,5 7,9 7,9 4,7 4,7 5,8 5,8 6,7 6,7 5,6 5,6 4,4 4,4 3,3 3,3 2,2 2,2 3)),((5 2,4 2,4 3,6 3,6 2,5 2)),((7 2,7 3,8 3,8 2,8 1,7 1,7 2)))" 
};

static std::string case_78_multi[2] = 
{
    "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0),(2 2,4 2,4 3,2 3,2 2)))",
    "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0),(3 2,4 2,4 3,3 3,3 2),(1 1,2 1,2 2,1 2,1 1)))" 

};

static std::string case_80_multi[2] = 
{
    // Many ux-clusters -> needs correct cluster-sorting
    // Fixed now
    "MULTIPOLYGON(((3 1,3 2,4 2,3 1)),((1 5,0 4,0 5,1 6,1 5)),((3 3,4 3,3 2,2 2,2 3,3 3)),((4 5,5 6,5 5,4 5)),((4 2,4 3,5 3,4 2)),((2.5 5.5,3 5,2 5,2 7,3 6,2.5 5.5)),((1 6,0 6,0 7,1 7,2 6,1 6)))",
    "MULTIPOLYGON(((3 5,3 6,4 6,4 5,3 5)),((4 4,5 5,5 4,4 4)),((3 3,4 4,4 3,3 3)),((1 5,1 6,2 6,2 5,1 5)),((0 6,1 7,1 6,0 6)),((1 4,1 3,0 3,0 4,1 4)),((3 5,4 4,3 4,3 3,2 3,2 5,3 5)))" 
};

static std::string case_81_multi[2] = 
{
    "MULTIPOLYGON(((1 1,2 2,2 1,1 1)),((2 2,2 3,3 2,2 2)),((3 1,4 2,4 1,3 1)))",
    "MULTIPOLYGON(((2 1,2 2,3 3,3 2,4 2,3 1,2 1)))" 
};

static std::string case_82_multi[2] = 
{
    "MULTIPOLYGON(((4 0,5 1,5 0,4 0)),((2 1,3 2,3 1,2 1)),((3 0,4 1,4 0,3 0)),((1 0,1 1,2 1,2 0,1 0)))",
    "MULTIPOLYGON(((3 2,4 3,4 2,3 2)),((3 1,3 2,4 1,3 1)),((0 0,1 1,1 0,0 0)),((5 1,5 0,4 0,4 1,5 1)))" 
};

static std::string case_83_multi[2] = 
{
    // iu/iu
    "MULTIPOLYGON(((1 0,1 1,2 1,1 0)),((0 1,0 4,1 4,1 1,0 1)),((2 1,2 2,3 2,3 1,2 1)),((2 3,3 4,3 3,2 3)))",
    "MULTIPOLYGON(((1 0,2 1,2 0,1 0)),((0 3,1 4,1 3,0 3)),((2 3,2 4,3 3,2 3)),((1 3,2 3,2 2,0 2,1 3)))" 
};

static std::string case_84_multi[2] = 
{
    // iu/ux
    "MULTIPOLYGON(((2 2,3 3,3 2,2 2)),((2 1,2 2,3 1,2 1)),((2 3,3 4,3 3,2 3)),((1 3,2 4,2 2,1 2,1 3)))",
    "MULTIPOLYGON(((2 3,3 3,3 1,2 1,2 2,1 2,1 3,2 3)))" 
};

static std::string case_85_multi[2] = 
{
    // iu/ux (and ux/ux)
    "MULTIPOLYGON(((0 1,0 2,1 2,0 1)),((1 1,1 2,2 1,1 1)),((0 3,1 3,0 2,0 3)))",
    "MULTIPOLYGON(((1 3,2 3,2 1,1 1,1 2,0 2,1 3)))" 
};

static std::string case_86_multi[2] = 
{
    // iu/ux
    "MULTIPOLYGON(((4 2,4 3,5 3,4 2)),((5 2,6 3,6 2,5 2)),((5 1,4 1,4 2,5 2,6 1,5 1)))",
    "MULTIPOLYGON(((5 1,5 2,6 2,6 1,5 1)),((4 2,5 3,5 2,4 2)),((3 2,4 3,4 2,3 2)))" 
};

static std::string case_87_multi[2] = 
{
    // iu/ux where iu crosses, no touch
    "MULTIPOLYGON(((5 0,5 1,6 0,5 0)),((6 2,7 3,7 2,6 2)),((5 1,5 3,6 3,6 1,5 1)))",
    "MULTIPOLYGON(((5 1,5 2,7 2,7 1,6 1,6 0,5 0,5 1)),((4 3,5 3,5 2,3 2,4 3)))" 
};


static std::string case_88_multi[2] = 
{
    "MULTIPOLYGON(((0 0,0 1,1 0,0 0)),((1 1,1 2,2 1,1 1)),((0 2,0 3,1 3,2 3,2 2,1 2,0 1,0 2)))",
    "MULTIPOLYGON(((0 0,0 1,1 0,0 0)),((0 1,1 2,1 1,0 1)),((0 2,0 3,1 3,1 2,0 2)))" 
};

static std::string case_89_multi[2] = 
{
    // Extract from rec.boxes_3
    "MULTIPOLYGON(((8 1,7 1,8 2,8 3,9 4,9 2,8.5 1.5,9 1,8 0,8 1)),((9 1,9 2,10 2,10 1,9 0,9 1)))",
    "MULTIPOLYGON(((8 3,9 4,9 3,8 3)),((7 0,7 1,8 1,8 0,7 0)),((9 2,9 1,8 1,8 3,8.5 2.5,9 3,9 2)))" 
};

static std::string case_90_multi[2] = 
{
    // iu/iu for Union; see ppt
    "MULTIPOLYGON(((1 8,0 8,0 10,1 10,1 9,2 8,2 7,1 7,1 8)),((2 9,2 10,4 10,4 9,3 9,3 8,2 8,2 9)))",
    "MULTIPOLYGON(((2 8,1 8,1 9,2 9,2 10,3 10,3 8,2 8)),((0 10,2 10,0 8,0 10)))" 
};

static std::string case_91_multi[2] = 
{
    // iu/xi for Intersection
    "MULTIPOLYGON(((3 3,3 4,4 4,3 3)),((2 2,1 2,1 4,2 4,2 3,3 3,2 2)))",
    "MULTIPOLYGON(((2 2,2 3,3 2,2 2)),((2 3,1 3,1 4,1.5 3.5,2 4,2.5 3.5,3 4,3 3,2 3)))" 
};

static std::string case_92_multi[2] = 
{
    // iu/iu all aligned (for union)
    "MULTIPOLYGON(((7 2,7 3,8 2,7 2)),((8 4,9 5,9 4,8 4)),((8 2,8 3,9 2,8 2)),((7 3,7 4,8 4,8 3,7 3)),((9 3,9 4,10 4,10 3,9 3)))",
    "MULTIPOLYGON(((9 2,8 2,8 3,9 3,10 2,9 2)),((7 5,8 5,9 6,9 4,8 4,7 3,6 3,6 4,6.5 3.5,7 4,6 4,7 5)))" 
};

static std::string case_93_multi[2] = 
{
    // iu/xi for intersection
    "MULTIPOLYGON(((6 2,7 2,7 1,5 1,6 2)),((7 3,8 3,7.5 2.5,8 2,7 2,7 3)))",
    "MULTIPOLYGON(((7 1,6 0,6 2,7 3,7 2,8 3,8 2,7 1)))" 
};


static std::string case_94_multi[2] = 
{
    // iu/iu for union
    "MULTIPOLYGON(((9 2,9 3,10 3,10 2,9 2)),((7 3,8 4,9 3,8 3,9 2,7 2,7 3)),((8 6,9 5,9 4,8 4,8 6)))",
    "MULTIPOLYGON(((6 2,6 3,7 3,8 2,6 2)),((9 3,10 3,9 2,9 1,8 0,7 0,8 1,8 3,8.5 2.5,9 3)),((7 4,7 5,8 5,9 6,9 4,8 4,8 3,7 3,7 4)))" 
};

static std::string case_95_multi[2] = 
{
    // iu/iu for union
    "MULTIPOLYGON(((0 8,1 8,1 7,0 7,0 8)),((2 8,2 9,2.5 8.5,3 9,3 7,2 7,2 8)),((1 9,1 10,2 9,1 8,1 9)))",
    "MULTIPOLYGON(((1 7,0 7,0 8,1 8,2 7,1 7)),((2 9,1 9,1 10,2 10,3 9,4 9,4 8,2 8,2 9)))" 
};
 
static std::string case_96_multi[2] = 
{
    // iu/iu all collinear, for intersection/union
    "MULTIPOLYGON(((8 2,9 3,9 2,8 2)),((8 1,9 2,9 1,10 1,10 0,8 0,8 1)))",
    "MULTIPOLYGON(((9 0,9 1,10 0,9 0)),((8 1,8 2,9 2,9 1,8 1)))" 
};

static std::string case_97_multi[2] = 
{
    // ux/ux for union
    "MULTIPOLYGON(((4 4,4 5,4.5 4.5,5 5,6 5,5 4,5 3,4 3,4 4)))",
    "MULTIPOLYGON(((5 3,5 4,6 3,5 3)),((6 5,7 5,6 4,5 4,6 5)))" 
};
 

static std::string case_98_multi[2] = 
{
    // ii/iu for intersection, solved by discarding iu (ordering not possible)
    "MULTIPOLYGON(((2 0,3 1,3 0,2 0)),((2 2,2 3,1 3,1 4,2 4,3 3,3 4,5 2,4 2,4 1,3 1,3 2,2.5 1.5,3 1,2 1,2 2)))",
    "MULTIPOLYGON(((4 2,4 3,5 2,4 2)),((1 0,0 0,0 2,4 2,4 1,2 1,2 0,1 0)),((3 3,4 4,4 3,3 2,3 3)))" 
};

static std::string case_99_multi[2] = 
{
    // iu/iu for intersection
    "MULTIPOLYGON(((1 0,2 1,2 0,1 0)),((1 2,2 2,1.5 1.5,2 1,1 1,1 0,0 0,0 1,1 2)))",
    "MULTIPOLYGON(((1 1,2 0,0 0,1 1)),((1 1,0 1,0 2,1 2,2 3,2 2,1 1)))" 
};

static std::string case_100_multi[2] = 
{
    // for intersection
    "MULTIPOLYGON(((0 0,0 1,1 0,0 0)),((2 2,2 1,0 1,0 2,1 2,2 3,2 2)))",
    "MULTIPOLYGON(((1 1,1 2,2 2,2 1,1 1)),((1 2,0 1,0 3,1 4,1 2)))" 
};

static std::string case_101_multi[2] = 
{
    // interior ring / union
    "MULTIPOLYGON(((7 2,7 3,8 2,7 2)),((9 3,9 4,10 3,9 3)),((10 1,10 0,8 0,8 1,9 2,10 2,10 1)),((9 3,9 2,8 2,8 3,7 3,7 4,8 4,9 3)),((8 4,8 7,9 6,9 4,8 4)))",
    "MULTIPOLYGON(((6 1,5 1,5 2,6 3,6 4,7 5,6 5,7 6,8 6,8 5,9 5,8 4,9 4,9 5,10 5,10 1,8 1,8 3,7 3,7 2,6 2,7 1,8 1,7 0,5 0,5 1,5.5 0.5,6 1),(8.5 2.5,9 2,9 3,8.5 2.5)))" 
};

static std::string case_102_multi[2] = 
{
    // interior ring 'fit' / union
    "MULTIPOLYGON(((0 2,0 7,5 7,5 2,0 2),(4 3,4 6,1 6,2 5,1 5,1 4,3 4,4 3)),((3 4,3 5,4 5,3 4)),((2 5,3 6,3 5,2 4,2 5)))",
    "MULTIPOLYGON(((0 2,0 7,5 7,5 2,0 2),(2 4,3 5,2 5,2 4),(4 4,3 4,3 3,4 4),(4 5,4 6,3 6,4 5)))" 
};

static std::string case_103_multi[2] = 
{
    // interior ring 'fit' (ix) / union / assemble 
    "MULTIPOLYGON(((0 0,0 5,5 5,5 0,2 0,2 1,3 1,3 2,2 2,2 3,1 2,2 2,2 1,1 0,0 0)))",
    "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0),(2 1,2 2,1 1,2 1)))" 
};

static std::string case_104_multi[2] = 
{
    // interior ring 'fit' (ii) / union / assemble 
    "MULTIPOLYGON(((1 0,1 1,0 1,0 5,5 5,5 0,2 0,2 1,1 0),(2 2,3 3,2 3,2 2)))",
    "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0),(1 1,3 1,3 2,1 2,1 1)))" 
};

static std::string case_105_multi[2] = 
{
    // interior ring 'fit' () / union / assemble 
    "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0),(2 2,3 2,3 3,1 3,2 2)))",
    "MULTIPOLYGON(((0 0,0 5,5 5,5 0,0 0),(1 1,2 1,2 2,1 1),(2 1,3 1,3 2,2 1),(1 3,3 3,3 4,2 3,2 4,1 4,1 3)))" 
};

static std::string case_106_multi[2] = 
{
    // interior ring 'fit' () / union / assemble 
    "MULTIPOLYGON(((0 0,0 3,1 2,1 3,2 3,2 1,3 2,2 2,3 3,2 3,3 4,1 4,1 3,0 3,0 5,5 5,5 0,0 0)))",
    "MULTIPOLYGON(((0 0,0 5,1 5,1 4,2 4,2 5,3 5,3 3,4 4,5 4,5 0,2 0,3 1,2 1,2 3,1 3,2 2,1.5 1.5,2 1,1 1,0 0)),((1 0,2 1,2 0,1 0)))" 
};

static std::string case_107_multi[2] = 
{
    // For CCW polygon reports a iu/iu problem.
    "MULTIPOLYGON(((6 8,7 9,7 7,8 7,7 6,6 6,6 8)),((6.5 9.5,7 10,7 9,6 9,6 10,6.5 9.5)))",
    "MULTIPOLYGON(((5 7,6 8,6 10,7 9,8 10,8 8,7 8,6 7,6 6,5 7)))" 
};


static std::string case_recursive_boxes_1[2] = 
{
    // == 70
    // Used in blog. KEEP
    "MULTIPOLYGON(((1 0,0 0,0 1,1 1,1 2,0 2,0 4,2 4,2 5,3 5,3 6,1 6,1 5,0 5,0 10,9 10,9 9,7 9,7 8,6 8,6 7,8 7,8 6,9 6,9 4,8 4,8 5,7 5,7 6,6 6,6 5,5 5,5 4,4 4,4 3,5 3,5 2,7 2,7 3,6 3,6 4,8 4,8 3,10 3,10 0,6 0,6 1,5 1,5 0,1 0),(4 7,4 9,3 9,3 7,4 7),(8 1,9 1,9 2,8 2,8 1)),((10 7,10 6,9 6,9 7,8 7,8 8,9 8,9 9,10 9,10 7)))",
    "MULTIPOLYGON(((1 0,0 0,0 7,2 7,2 6,5 6,5 5,4 5,4 4,5 4,5 3,7 3,7 2,6 2,6 0,1 0),(2 1,2 2,3 2,3 3,1 3,1 1,2 1)),((7 0,7 2,10 2,10 0,9 0,9 1,8 1,8 0,7 0)),((6 4,6 6,5 6,5 7,6 7,6 8,5 8,5 7,3 7,3 9,2 9,2 8,1 8,1 10,4 10,4 9,6 9,6 10,10 10,10 9,9 9,9 8,10 8,10 6,9 6,9 5,10 5,10 3,7 3,7 4,6 4),(7 7,7 6,8 6,8 7,7 7)))" 
};

static std::string case_recursive_boxes_2[2] = 
{
    // Traversal problem; Many ii-cases -> formerly caused "Endless loop" 
    // So it appears that there might be more decisions than intersection points
    "MULTIPOLYGON(((1 0,0 0,0 4,1 4,1 5,0 5,0 10,3 10,3 9,4 9,4 10,6 10,6 9,5 9,5 8,6 8,6 9,7 9,7 10,10 10,10 0,1 0),(7 6,8 6,8 7,9 7,9 8,8 8,8 7,7 7,7 6),(9 1,9 2,8 2,8 1,9 1)))",
    "MULTIPOLYGON(((0 0,0 10,10 10,10 0,8 0,8 1,7 1,7 0,0 0),(7 3,6 3,6 2,7 2,7 3),(6 7,7 7,7 8,6 8,6 7)))" 
};


static std::string case_recursive_boxes_3[2] = 
{
    // Previously a iu/ux problem causing union to fail. 
    // For CCW polygon it also reports a iu/iu problem.
    // KEEP
    "MULTIPOLYGON(((8 3,9 4,9 3,8 3)),((5 9,5 10,6 10,5 9)),((2 0,2 1,3 0,2 0)),((2 5,2 6,3 6,3 5,2 5)),((2 2,1 2,1 3,2 3,3 2,3 1,2 1,2 2)),((6 8,7 9,7 7,8 7,7 6,6 6,6 8)),((4 6,5 7,5 6,4 6)),((4 8,4 9,5 9,5 8,4 8)),((0 3,1 4,1 3,0 3)),((8 7,9 8,9 7,8 7)),((9 6,9 7,10 7,9 6)),((7 0,8 1,8 0,7 0)),((0 4,0 5,1 5,1 4,0 4)),((4 2,5 3,5 2,4 1,4 2)),((4 10,4 9,2 9,3 10,4 10)),((5 2,6 3,7 3,7 2,6 2,6 1,5 0,5 2)),((5 3,4 3,4 4,2 4,4 6,4 5,4.5 4.5,6 6,6 5,7 4,5 4,5 3)),((10 2,9 1,9 3,10 2)),((8 4,7 4,8 5,7 5,7 6,9 6,9 5,10 5,10 4,8 4)),((1 7,0 7,0 8,1 8,1 7)),((1 10,2 10,1 9,0 9,0 10,1 10)),((6.5 9.5,7 10,7 9,6 9,6 10,6.5 9.5)),((8 8,8 9,10 9,9 8,8 8)))",
    "MULTIPOLYGON(((0 7,0 8,1 8,1 7,0 7)),((5 3,4 3,4 4,6 4,6 3,7 3,6 2,5 2,5 3)),((8 2,8 3,9 2,8 2)),((1 1,2 2,2 1,1 1)),((2 1,3 1,2 0,1 0,2 1)),((2 3,3 4,3 3,2 3)),((1 9,2 8,1 8,1 9)),((2 10,2 9,1 9,1 10,2 10)),((9 7,9 8,10 8,10 7,9 7)),((6 0,6 1,7 1,7 0,6 0)),((8 0,9 1,9 0,8 0)),((1 6,1 5,0 5,1 6)),((0 2,1 1,0 1,0 2)),((1 3,2 3,2 2,1 2,1 3)),((5 1,5 0,4 0,4 1,3 1,4 2,5 2,6 1,5 1)),((1 3,0 3,0 4,1 4,1 3)),((3 6,4 5,2 5,3 6)),((9 2,10 2,10 1,9 1,9 2)),((7 5,6 4,6 5,7 6,8 6,8 5,7 5)),((7 4,8 5,8.5 4.5,9 5,9 4,8 4,8.5 3.5,9 4,10 3,7 3,7 4)),((1 6,1 7,3 7,3 8,4 7,5 7,6 8,6 10,7 9,8 10,9 10,9 9,8 9,8 8,7 8,6 7,6 6,1 6)))" 
};


static std::string case_recursive_boxes_4[2] = 
{
    // Occurred after refactoring assemble
    "MULTIPOLYGON(((9 3,10 4,10 3,9 3)),((9 9,10 10,10 9,9 9)),((0 1,0 3,1 4,0 4,0 5,1 6,0 6,0 8,1 9,1 8,2 9,2 7,1.5 6.5,2.5 5.5,3 6,3 5,4 6,2 6,2 7,3 8,2 8,3 9,0 9,0 10,6 10,5.5 9.5,6 9,6 10,7 10,7.5 9.5,8 10,8 9,7 9,7 8,6 8,6.5 7.5,7 8,8 8,8 9,9 9,9.5 8.5,10 9,10 8,9.5 7.5,10 7,10 5,8 5,8 4,7 3,7 2,8 3,8 4,9 5,9 3,10 2,10 1,8 1,8.5 0.5,9 1,10 0,4 0,4 1,3 1,3 0,1 0,1 1,0 0,0 1),(4 2,4.5 1.5,5 2,5 4,4.5 3.5,5 3,4 3,4 2),(3 3,4 4,2 4,2.5 3.5,3 4,3 3),(6 4,6.5 3.5,7 4,6 4),(5 7,5 9,4 9,4 8,5 7)))",
    "MULTIPOLYGON(((1 0,2 1,2 0,1 0)),((7 9,7 10,8 10,7 9)),((1 0,0 0,0 3,1 3,2 2,2 3,1 3,1 4,2 4,2 5,1 4,0 4,0 8,1 7,1 6,2 7,1 7,1 9,0 9,0 10,7 10,6 9,6.5 8.5,7 9,8 9,9 8,8 8,9 7,9 6,10 7,10 5,9 5,9 4,10 5,10 0,7 0,8 1,7 1,6 0,3 0,3 1,1 1,1 0),(5 1,5.5 0.5,6 1,6 2,6.5 1.5,7 2,8 2,8 4,7 3,6 3,6 2,5 2,6 1,5 1),(4 4,5 4,5 5,4 4),(4 6,4 7,3 7,2 6,3 6,3 7,4 6),(6 5,6.5 4.5,7 5,6 5,7 6,7 7,6 7,6 5),(3.5 7.5,4 8,4 9,3 8,3.5 7.5)),((9 8,9 9,8 9,9 10,10 10,10 8,9 8)))" 
};

static std::string pie_21_7_21_0_3[2] = 
{
    "MULTIPOLYGON(((2500 2500,2500 3875,2855 3828,3187 3690,3472 3472,3690 3187,3828 2855,3875 2500,3828 2144,3690 1812,3472 1527,3187 1309,2855 1171,2499 1125,2144 1171,1812 1309,1527 1527,1309 1812,1171 2144,1125 2499,1171 2855,1309 3187,2500 2500)))",
    "MULTIPOLYGON(((2500 2500,1704 3295,1937 3474,2208 3586,2499 3625,2791 3586,3062 3474,3295 3295,2500 2500)),((2500 2500,3586 2791,3625 2500,3586 2208,2500 2500)))" 
};

static std::string pie_23_19_5_0_2[2] = 
{
    "MULTIPOLYGON(((2500 2500,2500 3875,2855 3828,3187 3690,3472 3472,3690 3187,3828 2855,3875 2500,3828 2144,3690 1812,3472 1527,3187 1309,2855 1171,2499 1125,2144 1171,1812 1309,1527 1527,1309 1812,1171 2144,1125 2499,1171 2855,1309 3187,1527 3472,1812 3690,2500 2500)))",
    "MULTIPOLYGON(((2500 2500,3586 2791,3625 2500,3586 2208,3474 1937,3295 1704,3062 1525,2791 1413,2499 1375,2208 1413,1937 1525,1704 1704,1525 1937,1413 2208,1375 2500,1413 2791,1525 3062,1704 3295,1937 3474,2208 3586,2500 2500)),((2500 2500,2791 3586,3062 3474,2500 2500)))" 
};

static std::string pie_7_14_5_0_7[2] = 
{
    "MULTIPOLYGON(((2500 2500,2500 3875,2855 3828,3187 3690,3472 3472,3690 3187,3828 2855,3875 2500,2500 2500)))",
    "MULTIPOLYGON(((2500 2500,3586 2791,3625 2500,3586 2208,3474 1937,3295 1704,3062 1525,2791 1413,2499 1375,2208 1413,1937 1525,1704 1704,1525 1937,1413 2208,1375 2500,2500 2500)),((2500 2500,1525 3062,1704 3295,1937 3474,2208 3586,2499 3625,2791 3586,3062 3474,2500 2500)))" 
};

static std::string pie_16_16_9_0_2[2] = 
{
    "MULTIPOLYGON(((2500 2500,2500 3875,2855 3828,3187 3690,3472 3472,3690 3187,3828 2855,3875 2500,3828 2144,3690 1812,3472 1527,3187 1309,2855 1171,2499 1125,2144 1171,1812 1309,1527 1527,2500 2500)))",
    "MULTIPOLYGON(((2500 2500,3295 1704,3062 1525,2791 1413,2499 1375,2208 1413,1937 1525,1704 1704,1525 1937,1413 2208,1375 2500,1413 2791,1525 3062,1704 3295,1937 3474,2208 3586,2499 3625,2500 2500)),((2500 2500,3062 3474,3295 3295,2500 2500)))" 
};

static std::string pie_7_2_1_0_15[2] = 
{
    "MULTIPOLYGON(((2500 2500,2500 3875,2855 3828,3187 3690,3472 3472,3690 3187,3828 2855,3875 2500,2500 2500)))",
    "MULTIPOLYGON(((2500 2500,2791 3586,3062 3474,2500 2500)),((2500 2500,3474 3062,3586 2791,3625 2500,3586 2208,3474 1937,3295 1704,3062 1525,2791 1413,2499 1375,2208 1413,1937 1525,1704 1704,1525 1937,1413 2208,1375 2500,2500 2500)))" 
};

// Case, not literally on this list but derived, to mix polygon/multipolygon in call to difference
static std::string ggl_list_20111025_vd[4] =
    {
    "POLYGON((0 0,0 4,4 0,0 0))",
    "POLYGON((10 0,10 5,15 0,10 0))",
    "MULTIPOLYGON(((0 0,0 4,4 0,0 0)))",
    "MULTIPOLYGON(((10 0,10 5,15 0,10 0)))"
    };

// Same, mail with other case with text "Say the MP is the 2 squares below and P is the blue-ish rectangle."    
static std::string ggl_list_20111025_vd_2[2] =
    {
    "POLYGON((5 0,5 4,8 4,8 0,5 0))",
    "MULTIPOLYGON(((0 0,0 2,2 2,2 0,0 0)),((4 0,4 2,6 2,6 0,4 0)))"
    };
    
// Mail of h2 indicating that reversed order (in second polygon) has ix/ix problems
static std::string ggl_list_20120915_h2[3] =
    {
        "MULTIPOLYGON(((-2 5, -1 5, 0 5, 2 5, 2 -2, 1 -2, 1 -1, 0 -1,0 0, -1 0, -2 0, -2 5)))", 
        "MULTIPOLYGON(((0 0, 1 0, 1 -1, 0 -1, 0 0)), ((-1 5, 0 5, 0 0, -1 0, -1 5)))",
        "MULTIPOLYGON(((-1 5, 0 5, 0 0, -1 0, -1 5)), ((0 0, 1 0, 1 -1, 0 -1, 0 0)))"
    };

// Mail of volker, about another problem, but this specific example is causing two-point inner rings polygons which should be discarded
// (condition of num_points in detail/overlay/convert_ring.hpp)
static std::string ggl_list_20120221_volker[2] =
    {
        "MULTIPOLYGON(((1032 2130,1032 1764,2052 2712,1032 2130)),((3234 2580,2558 2690,3234 2532,3234 2580)),((2558 2690,2136 2790,2052 2712,2136 2760,2558 2690)))",
        "MULTIPOLYGON(((3232 2532.469945355191,2136 2790,1032 1764,1032 1458,1032 1212,2136 2328,3232 2220.196721311475,3232 1056,1031 1056,1031 2856,3232 2856,3232 2532.469945355191),(3232 2412.426229508197,2136 2646,3232 2412.426229508197)))"
    };


#endif // BOOST_GEOMETRY_TEST_MULTI_OVERLAY_CASES_HPP
