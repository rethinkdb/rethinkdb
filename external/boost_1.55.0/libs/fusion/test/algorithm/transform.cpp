/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2007 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/type_traits/is_reference.hpp>

struct square
{
    template<typename Sig>
    struct result;

    template <typename T>
    struct result<square(T)>
    {
        typedef int type;
    };

    template <typename T>
    int operator()(T x) const
    {
        return x * x;
    }
};

struct add
{
    template<typename Sig>
    struct result;

    template <typename A, typename B>
    struct result<add(A, B)>
    {
        typedef int type;
    };

    template <typename A, typename B>
    int operator()(A a, B b) const
    {
        return a + b;
    }
};

struct unary_lvalue_transform
{
    template<typename Sig>
    struct result;

    template<typename T>
    struct result<unary_lvalue_transform(T&)>
    {
        typedef T* type;
    };

    template<typename T>
    T* operator()(T& t) const
    {
        return &t;
    }
};

int twice(int v)
{
    return v*2;
}

struct binary_lvalue_transform
{
    template<typename Sig>
    struct result;

    template<typename T0, typename T1>
    struct result<binary_lvalue_transform(T0&,T1&)>
    {
        typedef T0* type;
    };

    template<typename T0, typename T1>
    T0* operator()(T0& t0, T1&) const
    {
        return &t0;
    }
};

int
main()
{
    using namespace boost::fusion;
    using boost::mpl::range_c;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing the transform

    {
        typedef range_c<int, 5, 9> sequence_type;
        sequence_type sequence;
        std::cout << boost::fusion::transform(sequence, square()) << std::endl;
        BOOST_TEST((boost::fusion::transform(sequence, square()) == make_vector(25, 36, 49, 64)));
    }

    {
        typedef range_c<int, 5, 9> mpl_list1;
        std::cout << boost::fusion::transform(mpl_list1(), square()) << std::endl;
        BOOST_TEST((boost::fusion::transform(mpl_list1(), square()) == make_vector(25, 36, 49, 64)));
    }
    
    {
        vector<int, int, int> tup(1, 2, 3);
        std::cout << boost::fusion::transform(tup, square()) << std::endl;
        BOOST_TEST((boost::fusion::transform(tup, square()) == make_vector(1, 4, 9)));
    }

    {
        vector<int, int, int> tup1(1, 2, 3);
        vector<int, int, int> tup2(4, 5, 6);
        std::cout << boost::fusion::transform(tup1, tup2, add()) << std::endl;
        BOOST_TEST((boost::fusion::transform(tup1, tup2, add()) == make_vector(5, 7, 9)));
    }

    {
        // Unary transform that requires lvalues, just check compilation
        vector<int, int, int> tup1(1, 2, 3);
        BOOST_TEST(at_c<0>(boost::fusion::transform(tup1, unary_lvalue_transform())) == &at_c<0>(tup1));
        BOOST_TEST(*begin(boost::fusion::transform(tup1, unary_lvalue_transform())) == &at_c<0>(tup1));
    }

    {
        vector<int, int, int> tup1(1, 2, 3);
        vector<int, int, int> tup2(4, 5, 6);
        BOOST_TEST(at_c<0>(boost::fusion::transform(tup1, tup2, binary_lvalue_transform())) == &at_c<0>(tup1));
        BOOST_TEST(*begin(boost::fusion::transform(tup1, tup2, binary_lvalue_transform())) == &at_c<0>(tup1));
    }

    {
        vector<int, int, int> tup1(1, 2, 3);
        BOOST_TEST(boost::fusion::transform(tup1, twice) == make_vector(2,4,6));
    }


    return boost::report_errors();
}

