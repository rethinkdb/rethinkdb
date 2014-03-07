/*=============================================================================
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/functional/adapter/fused_procedure.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/noncopyable.hpp>
#include <boost/blank.hpp>

#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/vector.hpp>

namespace fusion = boost::fusion;
using boost::noncopyable;

int effect;

#define CHECK_EFFECT(t,e)        \
    {                            \
        effect = 1234567; t;     \
        BOOST_TEST(effect == e); \
    }

template <class Base = boost::blank>
struct test_func
    : Base
{
    template <typename T0, typename T1>
    int operator()(T0 const & x, T1 const & y) const
    {
        return effect = 1+x-y;
    }

    template <typename T0, typename T1>
    int operator()(T0 const & x, T1 const & y) 
    {
        return effect = 2+x-y;
    }

    template <typename T0, typename T1>
    int operator()(T0 & x, T1 & y) const
    {
        return effect = 3+x-y;
    }

    template <typename T0, typename T1>
    int operator()(T0 & x, T1 & y) 
    {
        return effect = 4+x-y;
    }
};

int main()
{
    test_func<noncopyable> f;
    fusion::fused_procedure< test_func<> > fused_proc;
    fusion::fused_procedure< test_func<noncopyable> & > fused_proc_ref(f);
    fusion::fused_procedure< test_func<> const > fused_proc_c;
    fusion::fused_procedure< test_func<> > const fused_proc_c2;
    fusion::fused_procedure< test_func<noncopyable> const & > fused_proc_c_ref(f);

    fusion::vector<int,char> lv_vec(1,'\004');
    CHECK_EFFECT(fused_proc(lv_vec), 1);
    CHECK_EFFECT(fused_proc_c(lv_vec), 0);
    CHECK_EFFECT(fused_proc_c2(lv_vec), 0);
    CHECK_EFFECT(fused_proc_ref(lv_vec), 1);
    CHECK_EFFECT(fused_proc_c_ref(lv_vec), 0);

    CHECK_EFFECT(fused_proc(fusion::make_vector(2,'\003')), 1);
    CHECK_EFFECT(fused_proc_c(fusion::make_vector(2,'\003')), 0);
    CHECK_EFFECT(fused_proc_c2(fusion::make_vector(2,'\003')), 0);
    CHECK_EFFECT(fused_proc_ref(fusion::make_vector(2,'\003')), 1);
    CHECK_EFFECT(fused_proc_c_ref(fusion::make_vector(2,'\003')), 0);

    return boost::report_errors();
}
 
