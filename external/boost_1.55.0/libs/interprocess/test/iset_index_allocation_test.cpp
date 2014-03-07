//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2006-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/indexes/iset_index.hpp>
#include "named_allocation_test_template.hpp"

int main ()
{
   using namespace boost::interprocess;
   if(!test::test_named_allocation<iset_index>()){
      return 1;
   }

   return 0;
}

#include <boost/interprocess/detail/config_end.hpp>
