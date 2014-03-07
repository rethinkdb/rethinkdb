/*=============================================================================
    Copyright (c) 2006-2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/fusion/functional/adapter/unfused_typed.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/blank.hpp>
#include <boost/noncopyable.hpp>

#include <boost/utility/result_of.hpp>

#include <boost/mpl/identity.hpp>
#include <boost/mpl/placeholders.hpp>

#include <boost/utility/result_of.hpp>

#include <boost/fusion/algorithm/iteration/fold.hpp>

namespace fusion = boost::fusion;
namespace mpl = boost::mpl;
using mpl::placeholders::_;

using boost::noncopyable;

typedef fusion::vector<> types0;
typedef fusion::vector<long &> types1;
typedef fusion::vector<long &,int,char> types3;

template <class Base = boost::blank>
struct test_func
    : Base
{
    template<typename T>
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
        long operator()(long value, T const & elem) const
        {
          return value + sizeof(T) * elem;
        }

        template <typename T>
        long operator()(long value, T & elem) const
        {
          elem += sizeof(T);
          return value;
        }
    };
};

void result_type_tests()
{
    using boost::is_same;

    typedef fusion::unfused_typed< test_func<>, types0 > t0;
    BOOST_TEST(( is_same< boost::result_of< t0 () >::type, long >::value ));
    typedef fusion::unfused_typed< test_func<>, types1 > t1;
    BOOST_TEST(( is_same< boost::result_of< t1 (int) >::type, long >::value ));
}

#if defined(BOOST_MSVC) && BOOST_MSVC < 1400
#   define BOOST_TEST_NO_VC71(cond) (void)((cond)?0:1)
#else
#   define BOOST_TEST_NO_VC71(cond) BOOST_TEST(cond)
#endif

void nullary_tests()
{
    test_func<noncopyable> f;
    fusion::unfused_typed< test_func<>, types0 > unfused_func;
    fusion::unfused_typed< test_func<noncopyable> &, types0 > unfused_func_ref(f);
    fusion::unfused_typed< test_func<> const, types0 > unfused_func_c;
    fusion::unfused_typed< test_func<>, types0 > const unfused_func_c2;
    fusion::unfused_typed< test_func<noncopyable> const &, types0 > unfused_func_c_ref(f);

    BOOST_TEST(unfused_func() == 100);
    BOOST_TEST(unfused_func_ref() == 100);
    BOOST_TEST(unfused_func_c() == 0);
    BOOST_TEST(unfused_func_c2() == 0);
    BOOST_TEST(unfused_func_c_ref() == 0);
}

void unary_tests()
{
    test_func<noncopyable> f;
    fusion::unfused_typed< test_func<>, types1 > unfused_func;
    fusion::unfused_typed< test_func<noncopyable> &, types1 > unfused_func_ref(f);
    fusion::unfused_typed< test_func<> const, types1 > unfused_func_c;
    fusion::unfused_typed< test_func<>, types1 > const unfused_func_c2;
    fusion::unfused_typed< test_func<noncopyable> const &, types1 > unfused_func_c_ref(f);

    long lvalue = 1;
    BOOST_TEST_NO_VC71(unfused_func(lvalue) == 100);
    BOOST_TEST(lvalue == 1 + 1*sizeof(lvalue));
    BOOST_TEST(unfused_func_ref(lvalue) == 100);
    BOOST_TEST(lvalue == 1 + 2*sizeof(lvalue));
    BOOST_TEST(unfused_func_c(lvalue) == 0);
    BOOST_TEST(lvalue == 1 + 3*sizeof(lvalue));
    BOOST_TEST(unfused_func_c2(lvalue) == 0);
    BOOST_TEST(lvalue == 1 + 4*sizeof(lvalue));
    BOOST_TEST(unfused_func_c_ref(lvalue) == 0);
    BOOST_TEST(lvalue == 1 + 5*sizeof(lvalue));
}

void ternary_tests()
{
    test_func<noncopyable> f;
    fusion::unfused_typed< test_func<>, types3 > unfused_func;
    fusion::unfused_typed< test_func<noncopyable> &, types3 > unfused_func_ref(f);
    fusion::unfused_typed< test_func<> const, types3 > unfused_func_c;
    fusion::unfused_typed< test_func<>, types3 > const unfused_func_c2;
    fusion::unfused_typed< test_func<noncopyable> const &, types3 > unfused_func_c_ref(f);

    long lvalue = 1;
    static const long expected = 2*sizeof(int) + 7*sizeof(char);
    BOOST_TEST_NO_VC71(unfused_func(lvalue,2,'\007') == 100 + expected);
    BOOST_TEST(lvalue == 1 + 1*sizeof(lvalue));
    BOOST_TEST(unfused_func_ref(lvalue,2,'\007') == 100 + expected);
    BOOST_TEST(lvalue == 1 + 2*sizeof(lvalue));
    BOOST_TEST(unfused_func_c(lvalue,2,'\007') == 0 + expected);
    BOOST_TEST(lvalue == 1 + 3*sizeof(lvalue));
    BOOST_TEST(unfused_func_c2(lvalue,2,'\007') == 0 + expected);
    BOOST_TEST(lvalue == 1 + 4*sizeof(lvalue));
    BOOST_TEST(unfused_func_c_ref(lvalue,2,'\007') == 0 + expected);
    BOOST_TEST(lvalue == 1 + 5*sizeof(lvalue));
}

int main()
{
    result_type_tests();
    nullary_tests();
    unary_tests();
    ternary_tests();

    return boost::report_errors();
}

