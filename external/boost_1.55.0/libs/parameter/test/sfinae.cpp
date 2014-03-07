// Copyright David Abrahams, Daniel Wallin 2003. Use, modification and 
// distribution is subject to the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/parameter.hpp>
#include <boost/parameter/match.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <string>
#include <boost/type_traits/is_convertible.hpp>

#if ! defined(BOOST_NO_SFINAE) && ! BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x592))
# include <boost/utility/enable_if.hpp>
# include <boost/type_traits/is_same.hpp>
#endif 

namespace test
{
  BOOST_PARAMETER_KEYWORD(keywords,name)
  BOOST_PARAMETER_KEYWORD(keywords,value)
  
  using namespace boost::parameter;

  struct f_parameters
    : parameters<
          optional<
              keywords::name
            , boost::is_convertible<boost::mpl::_, std::string>
          >
        , optional<
              keywords::value
            , boost::is_convertible<boost::mpl::_, float>
          >
      >
  {};

  // The use of assert_equal_string is just a nasty workaround for a
  // vc++ 6 ICE.
  void assert_equal_string(std::string x, std::string y)
  {
        BOOST_TEST(x == y);
  }
  
  template<class P>
  void f_impl(P const& p)
  {
      float v = p[value | 3.f];
      BOOST_TEST(v == 3.f);
      assert_equal_string(p[name | "bar"], "foo");
  }

  void f()
  {
      f_impl(f_parameters()());
  }

  template<class A0>
  void f(
      A0 const& a0
    , BOOST_PARAMETER_MATCH(f_parameters, (A0), args))
  {
      f_impl(args(a0));
  }

  template<class A0, class A1>
  void f(
      A0 const& a0, A1 const& a1
    , BOOST_PARAMETER_MATCH(f_parameters,(A0)(A1), args))
  {
      f_impl(args(a0, a1));
  }

#if ! defined(BOOST_NO_SFINAE) && ! BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x592))
  // On compilers that actually support SFINAE, add another overload
  // that is an equally good match and can only be in the overload set
  // when the others are not.  This tests that the SFINAE is actually
  // working.  On all other compilers we're just checking that
  // everything about SFINAE-enabled code will work, except of course
  // the SFINAE.
  template<class A0, class A1>
  typename boost::enable_if<boost::is_same<int,A0>, int>::type
  f(A0 const& a0, A1 const& a1)
  {
      return 0;
  }
#endif 
} // namespace test

int main()
{
    using test::name;
    using test::value;    
    using test::f;

    f("foo");
    f("foo", 3.f);
    f(value = 3.f, name = "foo");

#if ! defined(BOOST_NO_SFINAE) && ! BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x592))
    BOOST_TEST(f(3, 4) == 0);
#endif
    return boost::report_errors();
}

