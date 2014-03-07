//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/config.hpp>

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

int main()
{
   return 0;
}

#else

#include <boost/move/detail/config_begin.hpp>

//[how_works_example
#include <boost/move/core.hpp>
#include <iostream>

class sink_tester
{
   public: //conversions provided by BOOST_COPYABLE_AND_MOVABLE
   operator ::boost::rv<sink_tester>&()
      {  return *static_cast< ::boost::rv<sink_tester>* >(this);  }
   operator const ::boost::rv<sink_tester>&() const
      {  return *static_cast<const ::boost::rv<sink_tester>* >(this);  }
};

//Functions returning different r/lvalue types
      sink_tester    rvalue()       {  return sink_tester(); }
const sink_tester    const_rvalue() {  return sink_tester(); }
      sink_tester &  lvalue()       {  static sink_tester lv; return lv; }
const sink_tester &  const_lvalue() {  static const sink_tester clv = sink_tester(); return clv; }

//BOOST_RV_REF overload
void sink(::boost::rv<sink_tester> &)      { std::cout << "non-const rvalue catched" << std::endl; }
//BOOST_COPY_ASSIGN_REF overload
void sink(const ::boost::rv<sink_tester> &){ std::cout << "const (r-l)value catched" << std::endl; }
//Overload provided by BOOST_COPYABLE_AND_MOVABLE
void sink(sink_tester &)                   { std::cout << "non-const lvalue catched" << std::endl; }

int main()
{
   sink(const_rvalue());   //"const (r-l)value catched"
   sink(const_lvalue());   //"const (r-l)value catched"
   sink(lvalue());         //"non-const lvalue catched"
   sink(rvalue());         //"non-const rvalue catched"
   return 0;
}
//]

#include <boost/move/detail/config_end.hpp>

#endif
