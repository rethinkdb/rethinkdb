/*=============================================================================
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/functional/adapter/unfused.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/noncopyable.hpp>
#include <boost/blank.hpp>

#include <boost/mpl/identity.hpp>

#include <boost/utility/result_of.hpp>

#include <boost/fusion/sequence/intrinsic/empty.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>

namespace fusion = boost::fusion;
namespace mpl = boost::mpl;

using boost::noncopyable;

template <class Base = boost::blank>
struct test_func
    : Base
{
    template <typename Sig>
    struct result;

    template <class Self, class Seq> 
    struct result< Self(Seq) >
        : mpl::identity<long> 
    { };

    template <typename Seq>
    long operator()(Seq const & seq) const
    {
        long state = 0;
        return fusion::fold(seq, state, fold_op());
    }

    template <typename Seq>
    long operator()(Seq const & seq) 
    {
        long state = 100;
        return fusion::fold(seq, state, fold_op());
    }

  private:

    struct fold_op
    {
        typedef long result_type;

        template <typename T>
        long operator()(long value, T & elem) const
        {
          elem += sizeof(T);
          return value + elem;
        }
    };
};

void result_type_tests()
{
    using boost::is_same;

    typedef fusion::unfused< test_func<> > t;
    BOOST_TEST(( is_same< boost::result_of< t () >::type, long >::value ));
    BOOST_TEST(( is_same< boost::result_of< t (int) >::type, long >::value ));
}

int main()
{
    result_type_tests();

    test_func<noncopyable> f;
    fusion::unfused< test_func<> > unfused_func;
    fusion::unfused< test_func<noncopyable> & > unfused_func_ref(f);
    fusion::unfused< test_func<> const > unfused_func_c;
    fusion::unfused< test_func<> > const unfused_func_c2;
    fusion::unfused< test_func<noncopyable> const & > unfused_func_c_ref(f);

    BOOST_TEST(unfused_func() == 100);
    BOOST_TEST(unfused_func_ref() == 100);
    BOOST_TEST(unfused_func_c() == 0);
    BOOST_TEST(unfused_func_c2() == 0);
    BOOST_TEST(unfused_func_c_ref() == 0);

    long lv1 = 2; int lv2 = 3l; char lv3 = '\007'; 
    long expected;

    expected = lv1+sizeof(lv1) + lv2+sizeof(lv2) + lv3+sizeof(lv3);
    BOOST_TEST(unfused_func(lv1,lv2,lv3) == 100 + expected); 
    BOOST_TEST(lv1 == 2+1*sizeof(lv1) && lv2 == 3+1*sizeof(lv2) && lv3 == 7+1*sizeof(lv3));

    expected = lv1+sizeof(lv1) + lv2+sizeof(lv2) + lv3+sizeof(lv3);
    BOOST_TEST(unfused_func_ref(lv1,lv2,lv3) == 100 + expected); 
    BOOST_TEST(lv1 == 2+2*sizeof(lv1) && lv2 == 3+2*sizeof(lv2) && lv3 == 7+2*sizeof(lv3));

    expected = lv1+sizeof(lv1) + lv2+sizeof(lv2) + lv3+sizeof(lv3);
    BOOST_TEST(unfused_func_c(lv1,lv2,lv3) == 0 + expected); 
    BOOST_TEST(lv1 == 2+3*sizeof(lv1) && lv2 == 3+3*sizeof(lv2) && lv3 == 7+3*sizeof(lv3));

    expected = lv1+sizeof(lv1) + lv2+sizeof(lv2) + lv3+sizeof(lv3);
    BOOST_TEST(unfused_func_c2(lv1,lv2,lv3) == 0 + expected); 
    BOOST_TEST(lv1 == 2+4*sizeof(lv1) && lv2 == 3+4*sizeof(lv2) && lv3 == 7+4*sizeof(lv3));

    expected = lv1+sizeof(lv1) + lv2+sizeof(lv2) + lv3+sizeof(lv3);
    BOOST_TEST(unfused_func_c_ref(lv1,lv2,lv3) == 0 + expected); 
    BOOST_TEST(lv1 == 2+5*sizeof(lv1) && lv2 == 3+5*sizeof(lv2) && lv3 == 7+5*sizeof(lv3));

    return boost::report_errors();
}

