/*=============================================================================
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/functional/generation/make_fused_function_object.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/noncopyable.hpp>
#include <boost/blank.hpp>

#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/vector.hpp>

namespace fusion = boost::fusion;
using boost::noncopyable;
using boost::cref;
using boost::ref;

template <class Base = boost::blank>
struct test_func
    : Base
{
    template<typename T>
    struct result
    {
    };


    template<class Self, typename T0, typename T1>
    struct result< Self(T0, T1) >
    {
        typedef int type;
    };

    template <typename T0, typename T1>
    int operator()(T0 const & x, T1 const & y) const
    {
        return 1+x-y;
    }

    template <typename T0, typename T1>
    int operator()(T0 const & x, T1 const & y) 
    {
        return 2+x-y;
    }

    template <typename T0, typename T1>
    int operator()(T0 & x, T1 & y) const
    {
        return 3+x-y;
    }

    template <typename T0, typename T1>
    int operator()(T0 & x, T1 & y) 
    {
        return 4+x-y;
    }
};

template <typename T>
inline T const & const_(T const & t)
{
    return t;
}

int main()
{
    fusion::vector<int,char> lv_vec(1,'\004');
    test_func<> f;
    test_func<noncopyable> f_nc;
 
    boost::fusion::result_of::make_fused_function_object< test_func<> >::type fused_func
        = fusion::make_fused_function_object(f);

    BOOST_TEST(fused_func(lv_vec) == 1);
    BOOST_TEST(const_(fused_func)(lv_vec) == 0);
    BOOST_TEST(fusion::make_fused_function_object(const_(f))(lv_vec) == 1);
    BOOST_TEST(fusion::make_fused_function_object(ref(f_nc))(lv_vec) == 1);
    BOOST_TEST(fusion::make_fused_function_object(cref(f_nc))(lv_vec) == 0);

    BOOST_TEST(fused_func(fusion::make_vector(2,'\003')) == 1);
    BOOST_TEST(const_(fused_func)(fusion::make_vector(2,'\003')) == 0);
    BOOST_TEST(fusion::make_fused_function_object(const_(f))(fusion::make_vector(2,'\003')) == 1);
    BOOST_TEST(fusion::make_fused_function_object(ref(f_nc))(fusion::make_vector(2,'\003')) == 1);
    BOOST_TEST(fusion::make_fused_function_object(cref(f_nc))(fusion::make_vector(2,'\003')) == 0);

    return boost::report_errors();
}


 
