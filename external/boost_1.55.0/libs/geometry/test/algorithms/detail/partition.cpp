// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <geometry_test_common.hpp>

#include <algorithms/test_overlay.hpp>


#include <boost/geometry/geometry.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/point.hpp>

#include <boost/geometry/algorithms/detail/partition.hpp>

#include <boost/geometry/io/wkt/wkt.hpp>
#include <boost/geometry/multi/io/wkt/wkt.hpp>

#if defined(TEST_WITH_SVG)
# include <boost/geometry/io/svg/svg_mapper.hpp>
#endif

#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>


template <typename Box>
struct box_item
{
    int id;
    Box box;
    box_item(int i = 0, std::string const& wkt = "")
        : id(i)
    {
        if (! wkt.empty())
        {
            bg::read_wkt(wkt, box);
        }
    }
};


struct get_box
{
    template <typename Box, typename InputItem>
    static inline void apply(Box& total, InputItem const& item)
    {
        bg::expand(total, item.box);
    }
};

struct ovelaps_box
{
    template <typename Box, typename InputItem>
    static inline bool apply(Box const& box, InputItem const& item)
    {
        return ! bg::detail::disjoint::disjoint_box_box(box, item.box);
    }
};


template <typename Box>
struct box_visitor
{
    int count;
    typename bg::default_area_result<Box>::type area;

    box_visitor()
        : count(0)
        , area(0)
    {}

    template <typename Item>
    inline void apply(Item const& item1, Item const& item2)
    {
        if (bg::intersects(item1.box, item2.box))
        {
            Box b;
            bg::intersection(item1.box, item2.box, b);
            area += bg::area(b);
            count++;
        }
    }
};



template <typename Box>
void test_boxes(std::string const& wkt_box_list, double expected_area, int expected_count)
{
    std::vector<std::string> wkt_boxes;
    
    boost::split(wkt_boxes, wkt_box_list, boost::is_any_of(";"), boost::token_compress_on);

    typedef box_item<Box> sample;
    std::vector<sample> boxes;

    int index = 1;
    BOOST_FOREACH(std::string const& wkt, wkt_boxes)
    {
        boxes.push_back(sample(index++, wkt));
    }

    box_visitor<Box> visitor;
    bg::partition
        <
            Box, get_box, ovelaps_box
        >::apply(boxes, visitor, 1);

    BOOST_CHECK_CLOSE(visitor.area, expected_area, 0.001);
    BOOST_CHECK_EQUAL(visitor.count, expected_count);
}



struct point_item 
{
    point_item()
        : id(0)
    {}

    int id;
    double x;
    double y;
};

BOOST_GEOMETRY_REGISTER_POINT_2D(point_item, double, cs::cartesian, x, y)


struct get_point
{
    template <typename Box, typename InputItem>
    static inline void apply(Box& total, InputItem const& item)
    {
        bg::expand(total, item);
    }
};

struct ovelaps_point
{
    template <typename Box, typename InputItem>
    static inline bool apply(Box const& box, InputItem const& item)
    {
        return ! bg::disjoint(item, box);
    }
};


struct point_visitor
{
    int count;

    point_visitor() 
        : count(0)
    {}

    template <typename Item>
    inline void apply(Item const& item1, Item const& item2)
    {
        if (bg::equals(item1, item2))
        {
            count++;
        }
    }
};



void test_points(std::string const& wkt1, std::string const& wkt2, int expected_count)
{
    bg::model::multi_point<point_item> mp1, mp2;
    bg::read_wkt(wkt1, mp1);
    bg::read_wkt(wkt2, mp2);

    int id = 1;
    BOOST_FOREACH(point_item& p, mp1) 
    { p.id = id++; }
    id = 1;
    BOOST_FOREACH(point_item& p, mp2) 
    { p.id = id++; }

    point_visitor visitor;
    bg::partition
        <
            bg::model::box<point_item>, get_point, ovelaps_point
        >::apply(mp1, mp2, visitor, 1);

    BOOST_CHECK_EQUAL(visitor.count, expected_count);
}



template <typename P>
void test_all()
{
    typedef bg::model::box<P> box;

    test_boxes<box>(
        // 1           2             3               4             5             6             7
        "box(0 0,1 1); box(0 0,2 2); box(9 9,10 10); box(8 8,9 9); box(4 4,6 6); box(2 4,6 8); box(7 1,8 2)", 
        5, // Area(Intersection(1,2)) + A(I(5,6)) 
        3);

    test_boxes<box>(
        "box(0 0,10 10); box(4 4,6 6); box(3 3,7 7)", 
            4 + 16 + 4,  // A(I(1,2)) + A(I(1,3)) + A(I(2,3))
            3);

    test_boxes<box>(
        "box(0 2,10 3); box(3 1,4 5); box(7 1,8 5)", 
            1 + 1,  // A(I(1,2)) + A(I(1,3)) 
            2);

    test_points("multipoint((1 1))", "multipoint((1 1))", 1);
    test_points("multipoint((0 0),(1 1),(7 3),(10 10))", "multipoint((1 1),(2 2),(7 3))", 2);

}

//------------------- higher volumes

template <typename SvgMapper>
struct svg_visitor
{
    SvgMapper& m_mapper;

    svg_visitor(SvgMapper& mapper)
        : m_mapper(mapper)
    {}

    template <typename Box>
    inline void apply(Box const& box, int level) 
    {
        /*
        std::string color("rgb(64,64,64)");
        switch(level)
        {
            case 0 : color = "rgb(255,0,0)"; break;
            case 1 : color = "rgb(0,255,0)"; break;
            case 2 : color = "rgb(0,0,255)"; break;
            case 3 : color = "rgb(255,255,0)"; break;
            case 4 : color = "rgb(255,0,255)"; break;
            case 5 : color = "rgb(0,255,255)"; break;
            case 6 : color = "rgb(255,128,0)"; break;
            case 7 : color = "rgb(0,128,255)"; break;
        }
        std::ostringstream style;
        style << "fill:none;stroke-width:" << (5.0 - level / 2.0) << ";stroke:" << color << ";";
        m_mapper.map(box, style.str());
        */
        m_mapper.map(box, "fill:none;stroke-width:2;stroke:rgb(0,0,0);");

    }
};




template <typename Collection>
void fill_points(Collection& collection, int seed, int size, int count)
{
    typedef boost::minstd_rand base_generator_type;

    base_generator_type generator(seed);

    boost::uniform_int<> random_coordinate(0, size - 1);
    boost::variate_generator<base_generator_type&, boost::uniform_int<> >
        coordinate_generator(generator, random_coordinate);

    std::set<std::pair<int, int> > included;

    int n = 0;
    for (int i = 0; n < count && i < count*count; i++)
    {
        int x = coordinate_generator();
        int y = coordinate_generator();
        std::pair<int, int> pair = std::make_pair(x, y);
        if (included.find(pair) == included.end())
        {
            included.insert(pair);
            typename boost::range_value<Collection>::type item;
            item.x = x;
            item.y = y;
            collection.push_back(item);
            n++;
        }
    }
}

void test_many_points(int seed, int size, int count)
{
    bg::model::multi_point<point_item> mp1, mp2;

    fill_points(mp1, seed, size, count);
    fill_points(mp2, seed * 2, size, count);

    // Test equality in quadratic loop
    int expected_count = 0;
    BOOST_FOREACH(point_item const& item1, mp1)
    {
        BOOST_FOREACH(point_item const& item2, mp2)
        {
            if (bg::equals(item1, item2))
            {
                expected_count++;
            }
        }
    }

#if defined(TEST_WITH_SVG)
    std::ostringstream filename;
    filename << "partition" << seed << ".svg";
    std::ofstream svg(filename.str().c_str());

    bg::svg_mapper<point_item> mapper(svg, 800, 800);

    {
        point_item p; 
        p.x = -1; p.y = -1; mapper.add(p);
        p.x = size + 1; p.y = size + 1; mapper.add(p);
    }

    typedef svg_visitor<bg::svg_mapper<point_item> > box_visitor_type;
    box_visitor_type box_visitor(mapper);
#else
    typedef bg::visit_no_policy box_visitor_type;
    box_visitor_type box_visitor;
#endif

    point_visitor visitor;
    bg::partition
        <
            bg::model::box<point_item>, get_point, ovelaps_point,
            box_visitor_type
        >::apply(mp1, mp2, visitor, 2, box_visitor);

    BOOST_CHECK_EQUAL(visitor.count, expected_count);

#if defined(TEST_WITH_SVG)
    BOOST_FOREACH(point_item const& item, mp1)
    {
        mapper.map(item, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1", 8);
    }
    BOOST_FOREACH(point_item const& item, mp2)
    {
        mapper.map(item, "fill:rgb(0,128,255);stroke:rgb(0,0,100);stroke-width:1", 4);
    }
#endif
}

template <typename Collection>
void fill_boxes(Collection& collection, int seed, int size, int count)
{
    typedef boost::minstd_rand base_generator_type;

    base_generator_type generator(seed);

    boost::uniform_int<> random_coordinate(0, size * 10 - 1);
    boost::variate_generator<base_generator_type&, boost::uniform_int<> >
        coordinate_generator(generator, random_coordinate);

    int n = 0;
    for (int i = 0; n < count && i < count*count; i++)
    {
        int w = coordinate_generator() % 30;
        int h = coordinate_generator() % 30;
        if (w > 0 && h > 0)
        {
            int x = coordinate_generator();
            int y = coordinate_generator();
            if (x + w < size * 10 && y + h < size * 10)
            {
                typename boost::range_value<Collection>::type item(n+1);
                bg::assign_values(item.box, x / 10.0, y / 10.0, (x + w) / 10.0, (y + h) / 10.0);
                collection.push_back(item);
                n++;
            }
        }
    }
}



void test_many_boxes(int seed, int size, int count)
{
    typedef bg::model::box<point_item> box_type;
    std::vector<box_item<box_type> > boxes;

    fill_boxes(boxes, seed, size, count);

    // Test equality in quadratic loop
    int expected_count = 0;
    double expected_area = 0.0;
    BOOST_FOREACH(box_item<box_type> const& item1, boxes)
    {
        BOOST_FOREACH(box_item<box_type> const& item2, boxes)
        {
            if (item1.id < item2.id)
            {
                if (bg::intersects(item1.box, item2.box))
                {
                    box_type b;
                    bg::intersection(item1.box, item2.box, b);
                    expected_area += bg::area(b);
                    expected_count++;
                }
            }
        }
    }


#if defined(TEST_WITH_SVG)
    std::ostringstream filename;
    filename << "partition_box_" << seed << ".svg";
    std::ofstream svg(filename.str().c_str());

    bg::svg_mapper<point_item> mapper(svg, 800, 800);

    {
        point_item p; 
        p.x = -1; p.y = -1; mapper.add(p);
        p.x = size + 1; p.y = size + 1; mapper.add(p);
    }

    BOOST_FOREACH(box_item<box_type> const& item, boxes)
    {
        mapper.map(item.box, "opacity:0.6;fill:rgb(50,50,210);stroke:rgb(0,0,0);stroke-width:1");
    }

    typedef svg_visitor<bg::svg_mapper<point_item> > partition_visitor_type;
    partition_visitor_type partition_visitor(mapper);


    box_visitor<box_type> visitor;
    bg::partition
        <
            box_type, get_box, ovelaps_box,
            partition_visitor_type
        >::apply(boxes, visitor, 2, partition_visitor);

    BOOST_CHECK_EQUAL(visitor.count, expected_count);
    BOOST_CHECK_CLOSE(visitor.area, expected_area, 0.001);

#endif
}





int test_main( int , char* [] )
{
    test_all<bg::model::d2::point_xy<double> >();

    test_many_points(12345, 20, 40);
    test_many_points(54321, 20, 60);
    test_many_points(67890, 20, 80);
    test_many_points(98765, 20, 100);
    for (int i = 1; i < 10; i++)
    {
        test_many_points(i, 30, i * 20);
    }

    test_many_boxes(12345, 20, 40);
    for (int i = 1; i < 10; i++)
    {
        test_many_boxes(i, 20, i * 10);
    }

    return 0;
}
