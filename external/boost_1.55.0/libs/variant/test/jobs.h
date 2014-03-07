//-----------------------------------------------------------------------------
// boost-libs variant/libs/test/jobs.h header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003
// Eric Friedman, Itay Maman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _JOBSH_INC_
#define _JOBSH_INC_

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

#include "boost/variant/variant_fwd.hpp"
#include "boost/variant/get.hpp"
#include "boost/variant/apply_visitor.hpp"
#include "boost/variant/static_visitor.hpp"

#include "boost/detail/workaround.hpp"
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x0551))
#    pragma warn -lvc
#endif

struct to_text : boost::static_visitor<std::string>
{
private: // NO_FUNCTION_TEMPLATE_ORDERING workaround

    template < BOOST_VARIANT_ENUM_PARAMS(typename U) >
    std::string to_text_impl(
          const boost::variant< BOOST_VARIANT_ENUM_PARAMS(U) >& operand, long
        ) const
    {
        std::ostringstream ost;
        ost << "[V] " << boost::apply_visitor(to_text(), operand);

        return ost.str();
    }

    template <typename Value>
    std::string to_text_impl(const Value& operand, int) const
    {
        std::ostringstream ost;
        ost << "[V] " << operand;

        return ost.str();
    }

public:

    template <typename T>
    std::string operator()(const T& operand) const
    {
        return to_text_impl(operand, 1L);
    }

};

struct total_sizeof : boost::static_visitor<int>
{
   total_sizeof() : total_(0) { }

   template<class Value>
   int operator()(const Value&) const
   {
      total_ += sizeof(Value);
      return total_;
   }

   int result() const
   {
      return total_;
   }

   mutable int total_;

}; // total_sizeof



//Function object: sum_int
//Description: Compute total sum of a series of numbers, (when called successively)
//Use sizeof(T) if applied with a non-integral type
struct sum_int : boost::static_visitor<int>
{
   
   sum_int() : total_(0) { }


   template<int n>
   struct int_to_type
   {
      BOOST_STATIC_CONSTANT(int, value = n);
   }; 

   //Integral type - add numerical value
   template<typename T>
   void add(T t, int_to_type<true> ) const
   {
      total_ += t;
   }

   //Other types - add sizeof<T>
   template<typename T>
   void add(T& , int_to_type<false> ) const
   {
      total_ += sizeof(T);
   }

   template<typename T>
   int operator()(const T& t) const
   {
      //Int_to_type is used to select the correct add() overload
      add(t, int_to_type<boost::is_integral<T>::value>());
      return total_;
   }

   int result() const
   {
      return total_;
   }

private:
   mutable int total_;

}; //sum_int






//Function object: sum_double
//Description: Compute total sum of a series of numbers, (when called successively)
//Accpetable input types: float, double (Other types are silently ignored)
struct sum_double : boost::static_visitor<double>
{
   
   sum_double() : total_(0) { }

   void operator()(float value) const
   {
      total_ += value;
   }

   void operator()(double value) const
   {
      total_ += value;
   }

   template<typename T>
   void operator()(const T&) const
   {
      //Do nothing
   }

   double result() const
   {
      return total_;
   }

private:
   mutable double total_;

}; //sum_double



struct int_printer : boost::static_visitor<std::string>
{
   
   int_printer(std::string prefix_s = "") : prefix_s_(prefix_s) { }
   int_printer(const int_printer& other) : prefix_s_(other.prefix_s_)
   {
      ost_ << other.str();
   }

   std::string operator()(int x) const
   {
      ost_ << prefix_s_ << x;
      return str();
   }

   std::string operator()(const std::vector<int>& x) const
   {
      ost_ << prefix_s_;

      //Use another Int_printer object for printing a list of all integers
      int_printer job(",");
      ost_ << std::for_each(x.begin(), x.end(), job).str();
      
      return str();
   }

   std::string str() const
   {
      return ost_.str();
   }

private:
   std::string prefix_s_;
   mutable std::ostringstream ost_;
};  //int_printer


struct int_adder : boost::static_visitor<>
{
   
   int_adder(int rhs) : rhs_(rhs) { }

   result_type operator()(int& lhs) const
   {
      lhs += rhs_;
   }

   template<typename T>
   result_type operator()(const T& ) const
   {
      //Do nothing
   }

   int rhs_;
}; //int_adder



template<typename T>
struct spec 
{
   typedef T result;
};

template<typename VariantType, typename S>
inline void verify(VariantType& var, spec<S>, std::string str = "")
{
   const VariantType& cvar = var;

   BOOST_CHECK(boost::apply_visitor(total_sizeof(), cvar) == sizeof(S));
#if !defined(BOOST_NO_TYPEID)
   BOOST_CHECK(cvar.type() == typeid(S));
#endif

   //
   // Check get<>()
   //
   BOOST_CHECK(boost::get<S>(&var));
   BOOST_CHECK(boost::get<S>(&cvar));

   const S* ptr1 = 0;
   const S* ptr2 = 0;
   try
   {
      S& r = boost::get<S>(var);
      ptr1 = &r;
   }
   catch(boost::bad_get& )
   {
      BOOST_ERROR( "get<S> failed unexpectedly" );
   }

   try
   {
      const S& cr = boost::get<S>(cvar);
      ptr2 = &cr;
   }
   catch(boost::bad_get& )
   {
      BOOST_ERROR( "get<S> const failed unexpectedly" );
   }

   BOOST_CHECK(ptr1 != 0 && ptr2 == ptr1);

   //
   // Check string content
   //
   if(str.length() > 0)
   {
      std::string temp = boost::apply_visitor(to_text(), cvar);
      std::cout << "temp = " << temp << ", str = " << str << std::endl;
      BOOST_CHECK(temp == str);         
   }
}


template<typename VariantType, typename S>
inline void verify_not(VariantType& var, spec<S>)
{
   const VariantType& cvar = var;

#if !defined(BOOST_NO_TYPEID)
   BOOST_CHECK(cvar.type() != typeid(S));
#endif

   //
   // Check get<>()
   //
   BOOST_CHECK(!boost::get<S>(&var));
   BOOST_CHECK(!boost::get<S>(&cvar));

   const S* ptr1 = 0;
   const S* ptr2 = 0;
   try
   {
      S& r = boost::get<S>(var); // should throw
      BOOST_ERROR( "get<S> passed unexpectedly" );

      ptr1 = &r;
   }
   catch(boost::bad_get& )
   {
      // do nothing except pass-through
   }

   try
   {
      const S& cr = boost::get<S>(var); // should throw
      BOOST_ERROR( "get<S> const passed unexpectedly" );

      ptr2 = &cr;
   }
   catch(boost::bad_get& )
   {
      // do nothing except pass-through
   }

   BOOST_CHECK(ptr1 == 0 && ptr2 == 0);   
}


#endif //_JOBSH_INC_
