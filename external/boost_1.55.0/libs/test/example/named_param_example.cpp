//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Library Code
#include <boost/test/utils/named_params.hpp>

using namespace boost::nfp;

////////////////////////////////////////////////////////////////
// Example:

#include <iostream>
#include <boost/shared_ptr.hpp>

namespace test {
  typed_keyword<char const*,struct name_t>  name;
  typed_keyword<int,struct test_index_t>    index;
  keyword<struct value_t,true>              value;
  keyword<struct instance_t,true>           instance;
  keyword<struct ref_t>                     ref;

  template<typename ValueType>
  void foo1( char const* n, ValueType v, int i )
  {
      std::cout << n << '[' << i << "]=" << v << std::endl;
  }

  template<class Params>
  void foo(Params const& params)
  {
      int i = params[index];
      foo1( params[name], params[value], i );
  }

  template<class Params>
  void boo(Params const& params)
  {
      foo1( params[name], params[value], params.has(index) ? params[index] : 0 );
  }

  template<class Params>
  void doo(Params const& params)
  {
      char const* nm;
      if( params.has(name) )
          nm = params[name];
      else
          nm = "abc";
      foo1( nm, params[value], params.has(index) ? params[index] : 0 );
  }

  template<typename T>
  void moo1( T* t )
  {
      std::cout << "non shared " << *t << std::endl;
  }

  template<typename T>
  void moo1( boost::shared_ptr<T> const& t )
  {
      std::cout << "shared " << *t << std::endl;
  }

  template<class Params>
  void moo(Params const& params)
  {
      moo1( params[instance] );
  }

  template<class Params>
  void goo(Params const& params)
  {
      params[ref] = 6;
  }
}

int main()
{
   using test::foo;
   using test::boo;
   using test::moo;
   using test::doo;
   using test::goo;
   using test::name;
   using test::value;
   using test::index;
   using test::instance;
   using test::ref;

   foo(( name = "foo", index = 0, value = 2.5 ));
   foo(( value = 'a', index = 1, name = "foo" ));
   foo(( name = "foo", value = "abc", index = 1 ));

   try {
       foo(( name = "foo", value = "abc" ));
   }
   catch( nfp_detail::access_to_invalid_parameter const& ) {
       std::cout << "Got access_to_invalid_parameter" << std::endl;
   }

   boo(( name = "boo", value = "abc" ));
   boo(( name = "boo", index = 1, value = "abc" ));
   doo(( value = "abc" ));
   doo(( value = 1.56, name = "ytr" ));

   int i = 5;

   moo( instance = &i );
   moo( instance = boost::shared_ptr<float>( new float(1.2) ) );

   goo( ref = i );

   return 0;
}

// EOF
