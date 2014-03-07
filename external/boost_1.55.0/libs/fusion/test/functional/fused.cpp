/*=============================================================================
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/functional/adapter/fused.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/noncopyable.hpp>
#include <boost/blank.hpp>

#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/vector.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/assert.hpp>

namespace fusion = boost::fusion;
using boost::noncopyable;

template <class Base = boost::blank>
struct test_func
    : Base
{
    typedef int result_type;

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

    typedef fusion::fused< test_func<> > ff;
    ff fused_func;

    typedef fusion::fused< test_func<noncopyable> & > ffr;
    ffr fused_func_ref(f);

    typedef fusion::fused< test_func<> const > ffc;
    ffc fused_func_c;

    typedef fusion::fused< test_func<> > const ffc2;
    ffc2 fused_func_c2;

    typedef fusion::fused< test_func<noncopyable> const & > ffcr;
    ffcr fused_func_c_ref(f);

    typedef fusion::vector<int,char> vec;
    vec lv_vec(1,'\004');

    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<ff(vec)>::type, int>));
    BOOST_TEST(fused_func(lv_vec) == 1);
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<ffr(vec)>::type, int>));
    BOOST_TEST(fused_func_c(lv_vec) == 0);
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<ffc(vec)>::type, int>));
    BOOST_TEST(fused_func_c2(lv_vec) == 0);
    BOOST_TEST(fused_func_ref(lv_vec) == 1);
    BOOST_MPL_ASSERT((boost::is_same<boost::result_of<ffcr(vec)>::type, int>));
    BOOST_TEST(fused_func_c_ref(lv_vec) == 0);

    BOOST_TEST(fused_func(fusion::make_vector(2,'\003')) == 1);
    BOOST_TEST(fused_func_c(fusion::make_vector(2,'\003')) == 0);
    BOOST_TEST(fused_func_c2(fusion::make_vector(2,'\003')) == 0);
    BOOST_TEST(fused_func_ref(fusion::make_vector(2,'\003')) == 1);
    BOOST_TEST(fused_func_c_ref(fusion::make_vector(2,'\003')) == 0);

    return boost::report_errors();
}


 
