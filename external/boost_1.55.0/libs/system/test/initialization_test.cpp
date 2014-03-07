//  initialization_test.cpp  -------------------------------------------------//

//  Copyright Christoper Kohlhoff 2007

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/system

//  This test verifiies that the error_category vtable does not suffer from
//  order-of-initialization problems.

#include <boost/detail/lightweight_test.hpp>
#include <boost/system/error_code.hpp>

struct foo
{
  foo()
  {
    boost::system::error_code ec;
    ec == boost::system::posix::permission_denied;
  }
} f;

int main( int, char ** )
{
  return ::boost::report_errors();
}
