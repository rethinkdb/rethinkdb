//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright David Abrahams, Vicente Botet, Ion Gaztanaga 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/detail/config_begin.hpp>
#include <boost/move/iterator.hpp>
#include <boost/container/vector.hpp>
#include "../example/movable.hpp"

int main()
{
   namespace bc = ::boost::container;
   //Default construct 10 movable objects
   bc::vector<movable> v(10);

   //Test default constructed value
   if(v[0].moved()){
      return 1;
   }

   //Move values
   bc::vector<movable> v2
      (boost::make_move_iterator(v.begin()), boost::make_move_iterator(v.end()));

   //Test values have been moved
   if(!v[0].moved()){
      return 1;
   }

   if(v2.size() != 10){
      return 1;
   }

   //Move again
   v.assign(boost::make_move_iterator(v2.begin()), boost::make_move_iterator(v2.end()));

   //Test values have been moved
   if(!v2[0].moved()){
      return 1;
   }

   if(v[0].moved()){
      return 1;
   }

   return 0;
}

#include <boost/move/detail/config_end.hpp>
