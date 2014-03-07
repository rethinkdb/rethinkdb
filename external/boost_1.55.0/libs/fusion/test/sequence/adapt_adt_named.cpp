/*=============================================================================
    Copyright (c) 2001-2009 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/adapted/adt/adapt_adt_named.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/empty.hpp>
#include <boost/fusion/sequence/intrinsic/front.hpp>
#include <boost/fusion/sequence/intrinsic/back.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/list/list.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/sequence/comparison/not_equal_to.hpp>
#include <boost/fusion/sequence/comparison/less.hpp>
#include <boost/fusion/sequence/comparison/less_equal.hpp>
#include <boost/fusion/sequence/comparison/greater.hpp>
#include <boost/fusion/sequence/comparison/greater_equal.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/fusion/support/is_view.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/assert.hpp>
#include <iostream>
#include <string>

namespace ns
{
    class point
    {
    public:
    
        point() : x(0), y(0) {}
        point(int in_x, int in_y) : x(in_x), y(in_y) {}
            
        int get_x() const { return x; }
        int get_y() const { return y; }
        void set_x(int x_) { x = x_; }
        void set_y(int y_) { y = y_; }
        
    private:
        
        int x;
        int y;
    };
}

// this creates a fusion view: boost::fusion::adapted::point
BOOST_FUSION_ADAPT_ADT_NAMED(
    ns::point, point,
    (int, int, obj.obj.get_x(), obj.obj.set_x(val))
    (int, int, obj.obj.get_y(), obj.obj.set_y(val))
)

int
main()
{
    using namespace boost::fusion;
    using namespace boost;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        BOOST_MPL_ASSERT((traits::is_view<adapted::point>));
        ns::point basep(123, 456);
        adapted::point p(basep);

        std::cout << at_c<0>(p) << std::endl;
        std::cout << at_c<1>(p) << std::endl;
        std::cout << p << std::endl;
        BOOST_TEST(p == make_vector(123, 456));

        at_c<0>(p) = 6;
        at_c<1>(p) = 9;
        BOOST_TEST(p == make_vector(6, 9));

        BOOST_STATIC_ASSERT(boost::fusion::result_of::size<adapted::point>::value == 2);
        BOOST_STATIC_ASSERT(!boost::fusion::result_of::empty<adapted::point>::value);

        BOOST_TEST(front(p) == 6);
        BOOST_TEST(back(p) == 9);
    }

    {
        fusion::vector<int, float> v1(4, 2);
        ns::point basep(5, 3);
        adapted::point v2(basep);

        fusion::vector<long, double> v3(5, 4);
        BOOST_TEST(v1 < v2);
        BOOST_TEST(v1 <= v2);
        BOOST_TEST(v2 > v1);
        BOOST_TEST(v2 >= v1);
        BOOST_TEST(v2 < v3);
        BOOST_TEST(v2 <= v3);
        BOOST_TEST(v3 > v2);
        BOOST_TEST(v3 >= v2);
    }

    {
        // conversion from ns::point to vector
        ns::point basep(5, 3);
        adapted::point p(basep);

        fusion::vector<int, long> v(p);
        v = p;
    }

    {
        // conversion from ns::point to list
        ns::point basep(5, 3);
        adapted::point p(basep);

        fusion::list<int, long> l(p);
        l = p;
    }

    {
        BOOST_MPL_ASSERT((mpl::is_sequence<adapted::point>));
        BOOST_MPL_ASSERT((boost::is_same<
            boost::fusion::result_of::value_at_c<adapted::point,0>::type
          , mpl::front<adapted::point>::type>));
    }

    return boost::report_errors();
}

