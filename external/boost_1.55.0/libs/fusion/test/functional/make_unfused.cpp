/*=============================================================================
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/functional/generation/make_unfused.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/noncopyable.hpp>
#include <boost/blank.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/identity.hpp>

#include <boost/utility/result_of.hpp>

#include <boost/fusion/sequence/intrinsic/empty.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>

#include <boost/ref.hpp>

namespace fusion = boost::fusion;
namespace mpl = boost::mpl;

using boost::noncopyable;
typedef mpl::true_ no_nullary_call;

using boost::ref;
using boost::cref;

template <class Base = boost::blank, class RemoveNullary = mpl::false_>
struct test_func
    : Base
{
    template <typename Sig>
    struct result;

    template <class Self, class Seq> 
    struct result< Self(Seq &) >
        : mpl::if_< mpl::and_< boost::fusion::result_of::empty<Seq>, RemoveNullary >, 
                    boost::blank, mpl::identity<long> >::type
    { };

    template <typename Seq>
    long operator()(Seq const & seq) const
    {
        long state = 0;
        return fusion::fold(seq, state, fold_op());
    }

    template < typename Seq >
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

template <typename T>
inline T const & const_(T const & t)
{
    return t;
}

int main()
{
    test_func<> f;
    test_func<noncopyable> f_nc;

    boost::fusion::result_of::make_unfused< test_func<> >::type unfused_func =
        fusion::make_unfused(f);

    boost::fusion::result_of::make_unfused< boost::reference_wrapper< 
        test_func<noncopyable> > >::type unfused_func_ref = 
            fusion::make_unfused(ref(f_nc));

    boost::fusion::result_of::make_unfused< boost::reference_wrapper< 
        test_func<noncopyable> const> >::type unfused_func_c_ref = 
            fusion::make_unfused(cref(f_nc));

    BOOST_TEST(unfused_func() == 100);
    BOOST_TEST(const_(unfused_func)() == 0);
    BOOST_TEST(unfused_func_ref() == 100);
    BOOST_TEST(unfused_func_c_ref() == 0);

    long lv1 = 2; int lv2 = 3l; char lv3 = '\007'; 
    long expected;

    expected = lv1+sizeof(lv1) + lv2+sizeof(lv2) + lv3+sizeof(lv3);
    BOOST_TEST(unfused_func(lv1,lv2,lv3) == 100 + expected); 
    BOOST_TEST(lv1 == 2+1*sizeof(lv1) && lv2 == 3+1*sizeof(lv2) && lv3 == 7+1*sizeof(lv3));

    expected = lv1+sizeof(lv1) + lv2+sizeof(lv2) + lv3+sizeof(lv3);
    BOOST_TEST(const_(unfused_func)(lv1,lv2,lv3) == 0 + expected); 
    BOOST_TEST(lv1 == 2+2*sizeof(lv1) && lv2 == 3+2*sizeof(lv2) && lv3 == 7+2*sizeof(lv3));

    expected = lv1+sizeof(lv1) + lv2+sizeof(lv2) + lv3+sizeof(lv3);
    BOOST_TEST(unfused_func_ref(lv1,lv2,lv3) == 100 + expected); 
    BOOST_TEST(lv1 == 2+3*sizeof(lv1) && lv2 == 3+3*sizeof(lv2) && lv3 == 7+3*sizeof(lv3));

    expected = lv1+sizeof(lv1) + lv2+sizeof(lv2) + lv3+sizeof(lv3);
    BOOST_TEST(unfused_func_c_ref(lv1,lv2,lv3) == 0 + expected); 
    BOOST_TEST(lv1 == 2+4*sizeof(lv1) && lv2 == 3+4*sizeof(lv2) && lv3 == 7+4*sizeof(lv3));

    return boost::report_errors();
}

