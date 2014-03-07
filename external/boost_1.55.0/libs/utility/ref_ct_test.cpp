// Copyright David Abrahams and Aleksey Gurtovoy
// 2002-2004. Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// compile-time test for "boost/ref.hpp" header content
// see 'ref_test.cpp' for run-time part

#include <boost/ref.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/mpl/assert.hpp>

namespace {

template< typename T, typename U >
void ref_test(boost::reference_wrapper<U>)
{
    typedef typename boost::reference_wrapper<U>::type type;
    BOOST_STATIC_ASSERT((boost::is_same<U,type>::value));
    BOOST_STATIC_ASSERT((boost::is_same<T,type>::value));
}

template< typename T >
void assignable_test(T x)
{
    x = x;
}

template< bool R, typename T >
void is_reference_wrapper_test(T)
{
    BOOST_STATIC_ASSERT(boost::is_reference_wrapper<T>::value == R);
}

template< typename R, typename Ref >
void cxx_reference_test(Ref)
{
#if BOOST_WORKAROUND(__BORLANDC__, < 0x600)
    typedef typename boost::remove_const<Ref>::type ref;
    BOOST_STATIC_ASSERT((boost::is_same<R,ref>::value));
#else
    BOOST_STATIC_ASSERT((boost::is_same<R,Ref>::value));
#endif
}

template< typename R, typename Ref >
void unwrap_reference_test(Ref)
{
#if BOOST_WORKAROUND(__BORLANDC__, < 0x600)
    typedef typename boost::remove_const<Ref>::type ref;
    typedef typename boost::unwrap_reference<ref>::type type;
#else
    typedef typename boost::unwrap_reference<Ref>::type type;
#endif
    BOOST_STATIC_ASSERT((boost::is_same<R,type>::value));
}

} // namespace

int main()
{
    int i = 0;
    int& ri = i;

    int const ci = 0;
    int const& rci = ci;

    // 'ref/cref' functions test
    ref_test<int>(boost::ref(i));
    ref_test<int>(boost::ref(ri));
    ref_test<int const>(boost::ref(ci));
    ref_test<int const>(boost::ref(rci));

    ref_test<int const>(boost::cref(i));
    ref_test<int const>(boost::cref(ri));
    ref_test<int const>(boost::cref(ci));
    ref_test<int const>(boost::cref(rci));

    // test 'assignable' requirement
    assignable_test(boost::ref(i));
    assignable_test(boost::ref(ri));
    assignable_test(boost::cref(i));
    assignable_test(boost::cref(ci));
    assignable_test(boost::cref(rci));

    // 'is_reference_wrapper' test
    is_reference_wrapper_test<true>(boost::ref(i));
    is_reference_wrapper_test<true>(boost::ref(ri));
    is_reference_wrapper_test<true>(boost::cref(i));
    is_reference_wrapper_test<true>(boost::cref(ci));
    is_reference_wrapper_test<true>(boost::cref(rci));

    is_reference_wrapper_test<false>(i);
    is_reference_wrapper_test<false, int&>(ri);
    is_reference_wrapper_test<false>(ci);
    is_reference_wrapper_test<false, int const&>(rci);

    // ordinary references/function template arguments deduction test
    cxx_reference_test<int>(i);
    cxx_reference_test<int>(ri);
    cxx_reference_test<int>(ci);
    cxx_reference_test<int>(rci);

    cxx_reference_test<int&, int&>(i);
    cxx_reference_test<int&, int&>(ri);
    cxx_reference_test<int const&, int const&>(i);
    cxx_reference_test<int const&, int const&>(ri);
    cxx_reference_test<int const&, int const&>(ci);
    cxx_reference_test<int const&, int const&>(rci);

    // 'unwrap_reference' test
    unwrap_reference_test<int>(boost::ref(i));
    unwrap_reference_test<int>(boost::ref(ri));
    unwrap_reference_test<int const>(boost::cref(i));
    unwrap_reference_test<int const>(boost::cref(ci));
    unwrap_reference_test<int const>(boost::cref(rci));

    unwrap_reference_test<int>(i);
    unwrap_reference_test<int>(ri);
    unwrap_reference_test<int>(ci);
    unwrap_reference_test<int>(rci);
    unwrap_reference_test<int&, int&>(i);
    unwrap_reference_test<int&, int&>(ri);
    unwrap_reference_test<int const&, int const&>(i);
    unwrap_reference_test<int const&, int const&>(ri);
    unwrap_reference_test<int const&, int const&>(ci);
    unwrap_reference_test<int const&, int const&>(rci);

    return 0;
}
