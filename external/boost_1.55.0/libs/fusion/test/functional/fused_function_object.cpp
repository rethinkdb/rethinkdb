/*=============================================================================
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/functional/adapter/fused_function_object.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/noncopyable.hpp>
#include <boost/blank.hpp>

#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/vector.hpp>

namespace fusion = boost::fusion;
using boost::noncopyable;

template <class Base = boost::blank>
struct test_func
    : Base
{
    template<typename T>
    struct result;

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

int main()
{
    test_func<noncopyable> f;
    fusion::fused_function_object< test_func<> > fused_func;
    fusion::fused_function_object< test_func<noncopyable> & > fused_func_ref(f);
    fusion::fused_function_object< test_func<> const > fused_func_c;
    fusion::fused_function_object< test_func<> > const fused_func_c2;
    fusion::fused_function_object< test_func<noncopyable> const & > fused_func_c_ref(f);

    fusion::vector<int,char> lv_vec(1,'\004');
    BOOST_TEST(fused_func(lv_vec) == 1);
    BOOST_TEST(fused_func_c(lv_vec) == 0);
    BOOST_TEST(fused_func_c2(lv_vec) == 0);
    BOOST_TEST(fused_func_ref(lv_vec) == 1);
    BOOST_TEST(fused_func_c_ref(lv_vec) == 0);

    BOOST_TEST(fused_func(fusion::make_vector(2,'\003')) == 1);
    BOOST_TEST(fused_func_c(fusion::make_vector(2,'\003')) == 0);
    BOOST_TEST(fused_func_c2(fusion::make_vector(2,'\003')) == 0);
    BOOST_TEST(fused_func_ref(fusion::make_vector(2,'\003')) == 1);
    BOOST_TEST(fused_func_c_ref(fusion::make_vector(2,'\003')) == 0);

    return boost::report_errors();
}
 
