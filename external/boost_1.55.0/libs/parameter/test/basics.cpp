// Copyright David Abrahams, Daniel Wallin 2003. Use, modification and 
// distribution is subject to the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter.hpp>
#include <cassert>
#include <string.h>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include "basics.hpp"

namespace test
{
  // A separate function for getting the "value" key, so we can deduce
  // F and use lazy_binding on it.
  template <class Params, class F>
  typename boost::parameter::lazy_binding<Params,tag::value,F>::type
  extract_value(Params const& p, F const& f)
  {
      typename boost::parameter::lazy_binding<
        Params, tag::value, F
      >::type v = p[value || f ];
      return v;
  }
  
  template<class Params>
  int f_impl(Params const& p)
  {
      typename boost::parameter::binding<Params, tag::name>::type
          n = p[name];

      typename boost::parameter::binding<
        Params, tag::value, double
      >::type v = extract_value(p, boost::bind(&value_default));
          
      typename boost::parameter::binding<
        Params, tag::index, int
      >::type i =
#if BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590042))
        p[test::index | 999];
#else
        p[index | 999];
#endif

      p[tester](n,v,i);

      return 1;
  }

  template<class Tester, class Name, class Value, class Index>
  int f(Tester const& t, const Name& name_, 
      const Value& value_, const Index& index_)
  {
      return f_impl(f_parameters()(t, name_, value_, index_));
  }

  template<class Tester, class Name, class Value>
  int f(Tester const& t, const Name& name_, const Value& value_)
  {
      return f_impl(f_parameters()(t, name_, value_));
  }

  template<class Tester, class Name>
  int f(Tester const& t, const Name& name_)
  {
      return f_impl(f_parameters()(t, name_));
  }

  template<class Params>
  int f_list(Params const& params)
  {
      return f_impl(params);
  }

}

int main()
{
   using test::f;
   using test::f_list;
   using test::name;
   using test::value;
   using test::index;
   using test::tester;

   f(
       test::values(S("foo"), S("bar"), S("baz"))
     , S("foo"), S("bar"), S("baz")
   );

   int x = 56;
   f(
       test::values("foo", 666.222, 56)
     , index = boost::ref(x), name = "foo"
   );

#if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))
   // No comma operator available on Borland
   f_list((
       tester = test::values("foo", 666.222, 56)
     , index = boost::ref(x)
     , name = "foo"
   ));
#endif
   
   //f(index = 56, name = 55); // won't compile

   return boost::report_errors();
}

