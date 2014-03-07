// ------------------------------------------------------------------------------
//  format_test_wstring.cpp :  test wchar_t format use (if supported)
// ------------------------------------------------------------------------------

//  Copyright Samuel Krempp 2003. Use, modification, and distribution are
//  subject to the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/format for library home page

// ------------------------------------------------------------------------------

#include "boost/format.hpp"

#define BOOST_INCLUDE_MAIN 
#include <boost/test/test_tools.hpp>


int test_main(int, char* [])
{

  using boost::format;
  using boost::str;

#if !defined(BOOST_NO_STD_WSTRING) && !defined(BOOST_NO_STD_WSTREAMBUF)
  using boost::wformat;
  wformat wfmter(L"%%##%%##%%1 %1%00");
  if(str( wfmter % L"Escaped OK" ) != L"%##%##%1 Escaped OK00")
      BOOST_ERROR("Basic w-parsing Failed");
  if(str( wformat(L"%%##%#x ##%%1 %s00") % 20 % L"Escaped OK" ) != L"%##0x14 ##%1 Escaped OK00")
      BOOST_ERROR("Basic wp-parsing Failed") ;
#endif // wformat tests


  return 0;
}
