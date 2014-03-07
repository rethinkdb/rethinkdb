//  (C) Copyright John Maddock 2002. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_DIRENT_H
//  TITLE:         <dirent.h>
//  DESCRIPTION:   The platform has an <dirent.h>.

#include <dirent.h>


namespace boost_has_dirent_h{

int test()
{
   DIR* pd = opendir("foobar");
   if(pd) closedir(pd);
   return 0;
}

}






