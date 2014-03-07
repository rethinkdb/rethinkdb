// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Custom polygon example

#include <iostream>

#include <boost/iterator.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>


#include <boost/geometry/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/geometry/util/add_const_if_c.hpp>

// Sample point, having x/y
struct my_point
{
    my_point(double a = 0, double b = 0)
        : x(a), y(b)
    {}
    double x,y;
};

// Sample polygon, having legacy methods
// (similar to e.g. COM objects)
class my_polygon
{
    std::vector<my_point> points;
    public :
        void add_point(my_point const& p) { points.push_back(p); }

        // Const access
        my_point const& get_point(std::size_t i) const
        {
            assert(i < points.size());
            return points[i];
        }

        // Mutable access
        my_point & get_point(std::size_t i)
        {
            assert(i < points.size());
            return points[i];
        }


        int point_count() const { return points.size(); }
        void erase_all() { points.clear(); }

        inline void set_size(int n) { points.resize(n); }
};

// ----------------------------------------------------------------------------
// Adaption: implement iterator and range-extension, and register with Boost.Geometry

// 1) implement iterator (const and non-const versions)
template<typename MyPolygon>
struct custom_iterator : public boost::iterator_facade
                            <
                                custom_iterator<MyPolygon>,
                                my_point,
                                boost::random_access_traversal_tag,
                                typename boost::mpl::if_
                                    <
                                        boost::is_const<MyPolygon>,
                                        my_point const,
                                        my_point
                                    >::type&
                            >
{
    // Constructor for begin()
    explicit custom_iterator(MyPolygon& polygon)
        : m_polygon(&polygon)
        , m_index(0)
    {}

    // Constructor for end()
    explicit custom_iterator(bool, MyPolygon& polygon)
        : m_polygon(&polygon)
        , m_index(polygon.point_count())
    {}


    // Default constructor
    explicit custom_iterator()
        : m_polygon(NULL)
        , m_index(-1)
    {}

    typedef typename boost::mpl::if_
        <
            boost::is_const<MyPolygon>,
            my_point const,
            my_point
        >::type my_point_type;

private:
    friend class boost::iterator_core_access;


    typedef boost::iterator_facade
        <
            custom_iterator<MyPolygon>,
            my_point,
            boost::random_access_traversal_tag,
            my_point_type&
        > facade;

    MyPolygon* m_polygon;
    int m_index;

    bool equal(custom_iterator const& other) const
    {
        return this->m_index == other.m_index;
    }
    typename facade::difference_type distance_to(custom_iterator const& other) const
    {
        return other.m_index - this->m_index;
    }

    void advance(typename facade::difference_type n)
    {
        m_index += n;
        if(m_polygon != NULL
            && (m_index >= m_polygon->point_count()
            || m_index < 0)
            )
        {
            m_index = m_polygon->point_count();
        }
    }

    void increment()
    {
        advance(1);
    }

    void decrement()
    {
        advance(-1);
    }

    // const and non-const dereference of this iterator
    my_point_type& dereference() const
    {
        return m_polygon->get_point(m_index);
    }
};




// 2) Implement Boost.Range const functionality
//    using method 2, "provide free-standing functions and specialize metafunctions"
// 2a) meta-functions
namespace boost
{
    template<> struct range_mutable_iterator<my_polygon>
    {
        typedef custom_iterator<my_polygon> type;
    };

    template<> struct range_const_iterator<my_polygon>
    {
        typedef custom_iterator<my_polygon const> type;
    };

    // RangeEx
    template<> struct range_size<my_polygon>
    {
        typedef std::size_t type;
    };

} // namespace 'boost'


// 2b) free-standing function for Boost.Range ADP
inline custom_iterator<my_polygon> range_begin(my_polygon& polygon)
{
    return custom_iterator<my_polygon>(polygon);
}

inline custom_iterator<my_polygon const> range_begin(my_polygon const& polygon)
{
    return custom_iterator<my_polygon const>(polygon);
}

inline custom_iterator<my_polygon> range_end(my_polygon& polygon)
{
    return custom_iterator<my_polygon>(true, polygon);
}

inline custom_iterator<my_polygon const> range_end(my_polygon const& polygon)
{
    return custom_iterator<my_polygon const>(true, polygon);
}



// 3) optional, for writable geometries only, implement push_back/resize/clear
namespace boost { namespace geometry { namespace traits
{

template<> struct push_back<my_polygon>
{
    static inline void apply(my_polygon& polygon, my_point const& point)
    {
        polygon.add_point(point);
    }
};

template<> struct resize<my_polygon>
{
    static inline void apply(my_polygon& polygon, std::size_t new_size)
    {
        polygon.set_size(new_size);
    }
};

template<> struct clear<my_polygon>
{
    static inline void apply(my_polygon& polygon)
    {
        polygon.erase_all();
    }
};

}}}


// 4) register with Boost.Geometry
BOOST_GEOMETRY_REGISTER_POINT_2D(my_point, double, cs::cartesian, x, y)

BOOST_GEOMETRY_REGISTER_RING(my_polygon)


// end adaption
// ----------------------------------------------------------------------------


void walk_using_iterator(my_polygon const& polygon)
{
    for (custom_iterator<my_polygon const> it = custom_iterator<my_polygon const>(polygon);
        it != custom_iterator<my_polygon const>(true, polygon);
        ++it)
    {
        std::cout << boost::geometry::dsv(*it) << std::endl;
    }
    std::cout << std::endl;
}


void walk_using_range(my_polygon const& polygon)
{
    for (boost::range_iterator<my_polygon const>::type it
            = boost::begin(polygon);
        it != boost::end(polygon);
        ++it)
    {
        std::cout << boost::geometry::dsv(*it) << std::endl;
    }
    std::cout << std::endl;
}


int main()
{
    my_polygon container1;

    // Create (as an example) a regular polygon
    const int n = 5;
    const double d = (360 / n) * boost::geometry::math::d2r;
    double a = 0;
    for (int i = 0; i < n + 1; i++, a += d)
    {
        container1.add_point(my_point(sin(a), cos(a)));
    }

    std::cout << "Walk using Boost.Iterator derivative" << std::endl;
    walk_using_iterator(container1);

    std::cout << "Walk using Boost.Range extension" << std::endl << std::endl;
    walk_using_range(container1);

    std::cout << "Use it by Boost.Geometry" << std::endl;
    std::cout << "Area: " << boost::geometry::area(container1) << std::endl;

    // Container 2 will be modified by Boost.Geometry. Add all points but the last one.
    my_polygon container2;
    for (int i = 0; i < n; i++)
    {
        // Use here the Boost.Geometry internal way of inserting (but the my_polygon way of getting)
        boost::geometry::traits::push_back<my_polygon>::apply(container2, container1.get_point(i));
    }

    std::cout << "Second container is not closed:" << std::endl;
    walk_using_range(container2);

    // Correct (= close it)
    boost::geometry::correct(container2);

    std::cout << "Now it is closed:" << std::endl;
    walk_using_range(container2);
    std::cout << "Area: " << boost::geometry::area(container2) << std::endl;

    // Use things from std:: using Boost.Range
    std::reverse(boost::begin(container2), boost::end(container2));
    std::cout << "Area reversed: " << boost::geometry::area(container2) << std::endl;

    return 0;
}
