// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#include <iostream>

namespace A
{
   namespace detail
   {
      template< typename T >
      int f( const T& x )
      {
         // Default:
         std::cout << 1 << std::endl;
         return 1;
      }
   
      template< typename T >
      int adl_f2( const T& x, int* )
      {
         return f( x );
      }
   
      template< typename T >
      int adl_f( const T& x )
      {
         return adl_f2( x, 0 );
      }
   }

   template< typename T >
   int f( const T& x )
   {
      return detail::adl_f( x );
   }

   template< typename T >
   int adl_f2( const T& x, int )
   {
      return detail::f( x );
   }
   
   //--------------------------------
   
   class C {};
/*
   // Optional:
   int f( const C& x )
   {
      std::cout << 2 << std::endl;
   }
*/
   template< typename T >
   class D {};
/*
   // Optional:
   template< typename T >
   int f( const D< T >& x )
   {
      std::cout << 3 << std::endl;
   }
   */
}

   
namespace B
{
   class C {};

   // Optional:
/*   int f( const C& )
   {
      std::cout << 4 << std::endl;
   }
*/
   template< typename T >
   class D {};
/*
   // Optional:
   template< typename T >
   int f( const D< T >& x )
   {
      std::cout << 5 << std::endl;
   }
   */
}

int main()
{
   A::f( 42 );

   A::C ac;
   A::f( ac );

   A::D< int > ad;
   A::f( ad );

   B::C bc;
   A::f( bc );

   B::D< int > bd;
   A::f( bd );
}
