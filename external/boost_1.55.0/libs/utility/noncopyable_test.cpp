//  boost class noncopyable test program  ------------------------------------//

//  (C) Copyright Beman Dawes 1999. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

//  Revision History
//   9 Jun 99  Add unnamed namespace
//   2 Jun 99  Initial Version

#include <boost/noncopyable.hpp>
#include <iostream>

//  This program demonstrates compiler errors resulting from trying to copy
//  construct or copy assign a class object derived from class noncopyable.

namespace
{
    class DontTreadOnMe : private boost::noncopyable
    {
    public:
         DontTreadOnMe() { std::cout << "defanged!" << std::endl; }
    };   // DontTreadOnMe

}   // unnamed namespace

int main()
{
    DontTreadOnMe object1;
    DontTreadOnMe object2(object1);
    object1 = object2;
    return 0;
}   // main
  
