
//  (C) Copyright John Maddock & Thorsten Ottosen 2005. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"
#ifdef TEST_STD
#  include <type_traits>
#else
#  include <boost/type_traits/decay.hpp>
#  include <boost/type_traits/is_same.hpp>
#endif
#include <iostream>
#include <string>
#include <utility>

#ifdef BOOST_INTEL
//  remark #383: value copied to temporary, reference to temporary used
//     std::pair<std::string, int>        p2 = boost::make_pair( "foo", 1 );
//                                                                      ^
#pragma warning(disable:383)
#endif

namespace boost
{

    int proc1()
    {
       return 0;
    }
    int proc2(int c)
    {
       return c;
    }
    
    //
    // An almost optimal version of std::make_pair()
    //
    template< class F, class S >
    inline std::pair< BOOST_DEDUCED_TYPENAME tt::decay<const F>::type, 
                      BOOST_DEDUCED_TYPENAME tt::decay<const S>::type >
    make_pair( const F& f, const S& s )
    {
        return std::pair< BOOST_DEDUCED_TYPENAME tt::decay<const F>::type, 
                          BOOST_DEDUCED_TYPENAME tt::decay<const S>::type >( f, s ); 
    }

    /*
    This overload will mess up vc7.1

    template< class F, class S >
    inline std::pair< BOOST_DEDUCED_TYPENAME ::tt::decay<F>::type, 
                      BOOST_DEDUCED_TYPENAME ::tt::decay<S>::type >
    make_pair( F& f, S& s )
    {
        return std::pair< BOOST_DEDUCED_TYPENAME ::tt::decay<F>::type, 
                          BOOST_DEDUCED_TYPENAME ::tt::decay<S>::type >( f, s ); 
    }
    */
}

TT_TEST_BEGIN(is_class)

   BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< 
          ::tt::decay<int>::type,int>::value),
                                  true );
   BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< 
          ::tt::decay<char[2]>::type,char*>::value),
                                 true );
   BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< 
          ::tt::decay<char[2][3]>::type,char(*)[3]>::value),
                                 true );
   BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< 
          ::tt::decay<const char[2]>::type,const char*>::value),
                                  true );
   BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< 
          ::tt::decay<wchar_t[2]>::type,wchar_t*>::value),
                                  true );
   BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< 
          ::tt::decay<const wchar_t[2]>::type,const wchar_t*>::value),
                                  true );
   BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< 
          ::tt::decay<const wchar_t[2]>::type,const wchar_t*>::value),
                                  true );

   typedef int f1_type(void);
   typedef int f2_type(int);

   BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< 
          ::tt::decay<f1_type>::type,int (*)(void)>::value),
                                  true );
   BOOST_CHECK_INTEGRAL_CONSTANT((::tt::is_same< 
          ::tt::decay<f2_type>::type,int (*)(int)>::value),
                                  true );

   std::pair<std::string,std::string> p  = boost::make_pair( "foo", "bar" );
   std::pair<std::string, int>        p2 = boost::make_pair( "foo", 1 );
#ifndef BOOST_NO_STD_WSTRING
   std::pair<std::wstring,std::string> p3  = boost::make_pair( L"foo", "bar" );
   std::pair<std::wstring, int>        p4  = boost::make_pair( L"foo", 1 );
#endif

   //
   // Todo: make these work sometime. The test id not directly
   //       related to decay<T>::type and can be avoided for now.
   // 
   /*
   int array[10];
   std::pair<int*,int*> p5 = boost::make_pair( array, array );
#ifndef __BORLANDC__
   std::pair<int(*)(void), int(*)(int)> p6 = boost::make_pair(boost::proc1, boost::proc2);
   p6.first();
   p6.second(1);
#endif
   */
   
TT_TEST_END








