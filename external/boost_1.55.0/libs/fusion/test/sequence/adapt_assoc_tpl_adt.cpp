/*=============================================================================
    Copyright (c) 2010 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/support.hpp>
#include <boost/fusion/container/list.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/adapted/adt/adapt_assoc_adt.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <iostream>
#include <string>

namespace ns
{
    struct x_member;
    struct y_member;
    struct z_member;

    template<typename X, typename Y>
    class point
    {
    public:
    
        point() : x(0), y(0) {}
        point(X in_x, Y in_y) : x(in_x), y(in_y) {}
            
        X get_x() const { return x; }
        Y get_y() const { return y; }
        void set_x(X x_) { x = x_; }
        void set_y(Y y_) { y = y_; }
        
    private:
        
        X x;
        Y y;
    };
}

BOOST_FUSION_ADAPT_ASSOC_TPL_ADT(
    (X)(Y),
    (ns::point)(X)(Y),
    (X, X, obj.get_x(), obj.set_x(val), ns::x_member)
    (Y, Y, obj.get_y(), obj.set_y(val), ns::y_member)
)

int
main()
{
    using namespace boost::fusion;

    typedef ns::point<int,int> point;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        BOOST_MPL_ASSERT_NOT((traits::is_view<point>));
        point p(123, 456);

        std::cout << at_c<0>(p) << std::endl;
        std::cout << at_c<1>(p) << std::endl;
        std::cout << p << std::endl;
        BOOST_TEST(p == make_vector(123, 456));

        at_c<0>(p) = 6;
        at_c<1>(p) = 9;
        BOOST_TEST(p == make_vector(6, 9));

        BOOST_STATIC_ASSERT(boost::fusion::result_of::size<point>::value == 2);
        BOOST_STATIC_ASSERT(!boost::fusion::result_of::empty<point>::value);

        BOOST_TEST(front(p) == 6);
        BOOST_TEST(back(p) == 9);
    }

    {
        boost::fusion::vector<int, float> v1(4, 2);
        point v2(5, 3);
        boost::fusion::vector<long, double> v3(5, 4);
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
        // conversion from point to vector
        point p(5, 3);
        boost::fusion::vector<int, long> v(p);
        v = p;
    }

    {
        // conversion from point to list
        point p(5, 3);
        boost::fusion::list<int, long> l(p);
        l = p;
    }

    {
        BOOST_MPL_ASSERT((boost::mpl::is_sequence<point>));
        BOOST_MPL_ASSERT((boost::is_same<
            boost::fusion::result_of::value_at_c<point,0>::type
          , boost::mpl::front<point>::type>));
    }

    {
        // assoc stuff
        BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<point, ns::x_member>));
        BOOST_MPL_ASSERT((boost::fusion::result_of::has_key<point, ns::y_member>));
        BOOST_MPL_ASSERT((boost::mpl::not_<boost::fusion::result_of::has_key<point, ns::z_member> >));

        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::value_at_key<point, ns::x_member>::type, int>));
        BOOST_MPL_ASSERT((boost::is_same<boost::fusion::result_of::value_at_key<point, ns::y_member>::type, int>));

        point p(5, 3);

        BOOST_TEST(at_key<ns::x_member>(p) == 5);
        BOOST_TEST(at_key<ns::y_member>(p) == 3);
    }

    return boost::report_errors();
}

