// Copyright David Abrahams and Aleksey Gurtovoy
// 2002-2004. Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// run-time test for "boost/ref.hpp" header content
// see 'ref_ct_test.cpp' for compile-time part

#if defined(_MSC_VER) && !defined(__ICL)
# pragma warning(disable: 4786)  // identifier truncated in debug info
# pragma warning(disable: 4710)  // function not inlined
# pragma warning(disable: 4711)  // function selected for automatic inline expansion
# pragma warning(disable: 4514)  // unreferenced inline removed
#endif

#include <boost/ref.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
# pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
# pragma warning(pop)
#endif


#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

namespace {
using namespace boost;

template <class T>
struct ref_wrapper
{
    // Used to verify implicit conversion
    static T* get_pointer(T& x)
    {
        return &x;
    }

    static T const* get_const_pointer(T const& x)
    {
        return &x;
    }

    template <class Arg>
    static T* passthru(Arg x)
    {
        return get_pointer(x);
    }

    template <class Arg>
    static T const* cref_passthru(Arg x)
    {
        return get_const_pointer(x);
    }

    static void test(T x)
    {
        BOOST_CHECK(passthru(ref(x)) == &x);
        BOOST_CHECK(&ref(x).get() == &x);

        BOOST_CHECK(cref_passthru(cref(x)) == &x);
        BOOST_CHECK(&cref(x).get() == &x);
    }
};

struct copy_counter {
  static int count_;
  copy_counter(copy_counter const& /*other*/) {
    ++count_;
  }
  copy_counter() {}
  static void reset() { count_ = 0; }
  static int count() { return copy_counter::count_;  }
};

int copy_counter::count_ = 0;

} // namespace unnamed

template <class T>
void do_unwrap(T t) {

  /* typename unwrap_reference<T>::type& lt = */
  unwrap_ref(t);

}

void unwrap_test() {

  int i = 3;
  const int ci = 2;

  do_unwrap(i);
  do_unwrap(ci);
  do_unwrap(ref(i));
  do_unwrap(cref(ci));
  do_unwrap(ref(ci));

  copy_counter cc;
  BOOST_CHECK(cc.count() == 0);

  do_unwrap(cc);
  do_unwrap(ref(cc));
  do_unwrap(cref(cc));

  BOOST_CHECK(cc.count() == 1);
  BOOST_CHECK(unwrap_ref(ref(cc)).count() == 1); 
}

int test_main(int, char * [])
{
    ref_wrapper<int>::test(1);
    ref_wrapper<int const>::test(1);
    unwrap_test();
    return 0;
}
